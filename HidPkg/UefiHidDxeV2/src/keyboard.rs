//! Provides Keyboard HID support.
//!
//! This module handles the core logic for processing keystrokes from HID
//! devices.
//!
//! ## License
//!
//! Copyright (C) Microsoft Corporation. All rights reserved.
//!
//! SPDX-License-Identifier: BSD-2-Clause-Patent
//!
mod key_queue;
mod simple_text_in;
mod simple_text_in_ex;

use alloc::{
    boxed::Box,
    collections::{BTreeMap, BTreeSet},
    vec,
    vec::Vec,
};
use core::sync::atomic::Ordering;
use core::{ffi::c_void, ptr};

use r_efi::{
    efi, hii,
    protocols::{
        self,
        simple_text_input_ex::{
            LEFT_ALT_PRESSED, LEFT_CONTROL_PRESSED, RIGHT_ALT_PRESSED, RIGHT_CONTROL_PRESSED, SHIFT_STATE_VALID,
        },
    },
};

use hidparser::{
    ArrayField, ReportDescriptor, ReportField, VariableField,
    report_data_types::{ReportId, Usage},
};
use mu_rust_helpers::function;
use rust_advanced_logger_dxe::{DEBUG_ERROR, DEBUG_VERBOSE, DEBUG_WARN, debugln};

use crate::{
    RUNTIME_SERVICES,
    boot_services::UefiBootServices,
    hid_io::{HidIo, HidReportReceiver},
    keyboard::key_queue::OrdKeyData,
};

// Repeat key delay: 500ms in 100ns units.
const REPEAT_KEY_DELAY: u64 = 5_000_000;
// Repeat key rate: 20ms in 100ns units (~50 keys/sec).
const REPEAT_KEY_RATE: u64 = 200_000;

// usages supported by this module
const KEYBOARD_MODIFIER_USAGE_MIN: u32 = 0x000700E0;
const KEYBOARD_MODIFIER_USAGE_MAX: u32 = 0x000700E7;
const KEYBOARD_USAGE_MIN: u32 = 0x00070001;
const KEYBOARD_USAGE_MAX: u32 = 0x00070065;
const LED_USAGE_MIN: u32 = 0x00080001;
const LED_USAGE_MAX: u32 = 0x00080005;

// maps a given field to a routine that handles input from it.
#[derive(Debug, Clone)]
struct ReportFieldWithHandler<T> {
    field: T,
    report_handler: fn(handler: &mut KeyboardHidHandler, field: T, report: &[u8]),
}

// maps a given field to a routine that builds output reports from it.
#[derive(Debug, Clone)]
struct ReportFieldBuilder<T> {
    field: T,
    field_builder: fn(&mut KeyboardHidHandler, field: T, report: &mut [u8]),
}

// Defines an input report and the fields of interest in it.
#[derive(Debug, Default, Clone)]
struct KeyboardReportData {
    report_id: Option<ReportId>,
    report_size: usize,
    relevant_variable_fields: Vec<ReportFieldWithHandler<VariableField>>,
    relevant_array_fields: Vec<ReportFieldWithHandler<ArrayField>>,
}

// Defines an output report and the fields of interest in it.
#[derive(Debug, Default, Clone)]
struct KeyboardOutputReportBuilder {
    report_id: Option<ReportId>,
    report_size: usize,
    relevant_variable_fields: Vec<ReportFieldBuilder<VariableField>>,
}

#[repr(C)]
struct LayoutChangeContext {
    boot_services: &'static dyn UefiBootServices,
    keyboard_handler: *mut KeyboardHidHandler,
}

#[repr(C)]
struct RepeatTimerContext {
    keyboard_handler: *mut KeyboardHidHandler,
}

/// Keyboard HID Handler
pub struct KeyboardHidHandler {
    boot_services: &'static dyn UefiBootServices,
    agent: efi::Handle,
    controller: Option<efi::Handle>,
    input_reports: BTreeMap<Option<ReportId>, KeyboardReportData>,
    output_builders: Vec<KeyboardOutputReportBuilder>,
    report_id_present: bool,
    last_keys: BTreeSet<Usage>,
    current_keys: BTreeSet<Usage>,
    led_state: BTreeSet<Usage>,
    key_queue: key_queue::KeyQueue,
    notification_callbacks: BTreeMap<usize, (OrdKeyData, protocols::simple_text_input_ex::KeyNotifyFunction)>,
    next_notify_handle: usize,
    key_notify_event: efi::Event,
    layout_change_event: efi::Event,
    layout_context: *mut LayoutChangeContext,
    repeat_timer_event: efi::Event,
    repeat_context: *mut RepeatTimerContext,
    repeat_key: Option<Usage>,
}

impl KeyboardHidHandler {
    /// Instantiates a new Keyboard HID handler. `agent` is the handle that owns the handler (typically image_handle)
    pub fn new(boot_services: &'static dyn UefiBootServices, agent: efi::Handle) -> Self {
        Self {
            boot_services,
            agent,
            controller: None,
            input_reports: BTreeMap::new(),
            output_builders: Vec::new(),
            report_id_present: false,
            last_keys: BTreeSet::new(),
            current_keys: BTreeSet::new(),
            led_state: BTreeSet::new(),
            key_queue: Default::default(),
            notification_callbacks: BTreeMap::new(),
            next_notify_handle: 0,
            key_notify_event: core::ptr::null_mut(),
            layout_change_event: core::ptr::null_mut(),
            layout_context: core::ptr::null_mut(),
            repeat_timer_event: core::ptr::null_mut(),
            repeat_context: core::ptr::null_mut(),
            repeat_key: None,
        }
    }

    // Processes the report descriptor to determine whether this is a supported device, and if so, extract the information
    // required to process reports.
    fn process_descriptor(&mut self, descriptor: ReportDescriptor) -> Result<(), efi::Status> {
        let multiple_reports =
            descriptor.input_reports.len() > 1 || descriptor.output_reports.len() > 1 || descriptor.features.len() > 1;

        for report in &descriptor.input_reports {
            let mut report_data = KeyboardReportData { report_id: report.report_id, ..Default::default() };

            self.report_id_present = report.report_id.is_some();

            if multiple_reports && !self.report_id_present {
                //Invalid to have None ReportId if multiple reports present.
                Err(efi::Status::DEVICE_ERROR)?;
            }

            report_data.report_size = report.size_in_bits.div_ceil(8);

            for field in &report.fields {
                match field {
                    //Variable fields (typically used for modifier Usages)
                    ReportField::Variable(field) => {
                        if let KEYBOARD_MODIFIER_USAGE_MIN..=KEYBOARD_MODIFIER_USAGE_MAX = field.usage.into() {
                            report_data.relevant_variable_fields.push(ReportFieldWithHandler::<VariableField> {
                                field: field.clone(),
                                report_handler: Self::handle_variable_key,
                            });
                        }
                    }
                    //Array fields (typically used for key strokes)
                    ReportField::Array(field) => {
                        for usage_list in &field.usage_list {
                            if usage_list.contains(Usage::from(KEYBOARD_USAGE_MIN))
                                || usage_list.contains(Usage::from(KEYBOARD_USAGE_MAX))
                            {
                                report_data.relevant_array_fields.push(ReportFieldWithHandler::<ArrayField> {
                                    field: field.clone(),
                                    report_handler: Self::handle_array_key,
                                });
                                break;
                            }
                        }
                    }
                    ReportField::Padding(_) => (), // padding irrelevant.
                }
            }
            if !(report_data.relevant_variable_fields.is_empty() && report_data.relevant_array_fields.is_empty()) {
                self.input_reports.insert(report_data.report_id, report_data);
            }
        }

        for report in &descriptor.output_reports {
            let mut report_builder = KeyboardOutputReportBuilder { report_id: report.report_id, ..Default::default() };

            self.report_id_present = report.report_id.is_some();

            if multiple_reports && !self.report_id_present {
                //invalid to have None ReportId if multiple reports present.
                Err(efi::Status::DEVICE_ERROR)?;
            }

            report_builder.report_size = usize::div_ceil(report.size_in_bits, 8);

            for field in &report.fields {
                match field {
        //Variable fields in output reports (typically used for LEDs).
        ReportField::Variable(field) => {
          if let LED_USAGE_MIN..=LED_USAGE_MAX = field.usage.into() {
            report_builder.relevant_variable_fields.push(
              ReportFieldBuilder {
                field: field.clone(),
                field_builder: Self::build_led_report
              }
            )
          }
        },
        ReportField::Array(_) | // No support for array field report outputs; could be added if required.
        ReportField::Padding(_) => (), // padding fields irrelevant.
      }
            }
            if !report_builder.relevant_variable_fields.is_empty() {
                self.output_builders.push(report_builder);
            }
        }

        if self.input_reports.is_empty() && self.output_builders.is_empty() {
            Err(efi::Status::UNSUPPORTED)
        } else {
            Ok(())
        }
    }

    // Helper routine to handle variable keyboard input report fields
    fn handle_variable_key(&mut self, field: VariableField, report: &[u8]) {
        match field.field_value(report) {
            Some(x) if x != 0 => _ = self.current_keys.insert(field.usage),
            _ => (),
        }
    }

    // Helper routine to handle array keyboard input report fields
    fn handle_array_key(&mut self, field: ArrayField, report: &[u8]) {
        match field.field_value(report) {
            Some(index) if index != 0 => {
                let mut index = (index as u32 - u32::from(field.logical_minimum)) as usize;
                let usage = field.usage_list.iter().find_map(|x| {
                    let range_size = (x.end() - x.start()) as usize;
                    if index <= range_size {
                        x.range().nth(index)
                    } else {
                        index -= range_size;
                        None
                    }
                });
                if let Some(usage) = usage {
                    self.current_keys.insert(Usage::from(usage));
                }
            }
            _ => (),
        }
    }

    // Helper routine that updates the fields in the given report buffer for the given field (called for each field for
    // every LED usage that was discovered in the output report descriptor).
    fn build_led_report(&mut self, field: VariableField, report: &mut [u8]) {
        let status = field.set_field_value(self.led_state.contains(&field.usage).into(), report);
        if status.is_err() {
            debugln!(DEBUG_WARN, "{:}: failed to set field value: {:?}", function!(), status);
        }
    }

    // Generates LED output reports - usually there is only one, but this implementation handles an arbitrary number of
    // possible output reports. If more than one output report is defined, all will be sent whenever there is a change in
    // LEDs.
    fn generate_led_output_reports(&mut self) -> Vec<(Option<ReportId>, Vec<u8>)> {
        let mut output_vec = Vec::new();
        let current_leds: BTreeSet<Usage> = self.key_queue.active_leds().iter().cloned().collect();
        self.led_state = current_leds;
        for output_builder in self.output_builders.clone() {
            let mut report_buffer = vec![0u8; output_builder.report_size];
            for field_builder in &output_builder.relevant_variable_fields {
                (field_builder.field_builder)(self, field_builder.field.clone(), report_buffer.as_mut_slice());
            }
            output_vec.push((output_builder.report_id, report_buffer));
        }
        output_vec
    }

    // Installs the FFI interfaces that provide keyboard support to the rest of the system
    fn install_protocol_interfaces(&mut self, controller: efi::Handle) -> Result<(), efi::Status> {
        simple_text_in::SimpleTextInFfi::install(self.boot_services, controller, self)?;
        simple_text_in_ex::SimpleTextInExFfi::install(self.boot_services, controller, self)?;
        self.controller = Some(controller);

        // after this point, access to self must be guarded by raising TPL to NOTIFY.
        Ok(())
    }

    // Installs an event to be notified when a new layout is installed. This allows the driver to respond dynamically to
    // installation of new layouts and handle keys accordingly.
    fn install_layout_change_event(&mut self) -> Result<(), efi::Status> {
        let context = LayoutChangeContext { boot_services: self.boot_services, keyboard_handler: self as *mut Self };
        let context_ptr = Box::into_raw(Box::new(context));

        let mut layout_change_event: efi::Event = ptr::null_mut();
        let status = self.boot_services.create_event_ex(
            efi::EVT_NOTIFY_SIGNAL,
            efi::TPL_NOTIFY,
            Some(on_layout_update),
            context_ptr as *mut c_void,
            &protocols::hii_database::SET_KEYBOARD_LAYOUT_EVENT_GUID,
            ptr::addr_of_mut!(layout_change_event),
        );
        if status.is_error() {
            Err(status)?;
        }

        self.layout_change_event = layout_change_event;
        self.layout_context = context_ptr;

        Ok(())
    }

    // Closes the layout change event.
    fn uninstall_layout_change_event(&mut self) -> Result<(), efi::Status> {
        if !self.layout_change_event.is_null() {
            let layout_change_event: efi::Handle = self.layout_change_event;
            let status = self.boot_services.close_event(layout_change_event);
            if status.is_error() {
                //An error here means the event was not closed, so in theory the notification_callback on it could still be fired.
                //Mark the instance invalid by setting the keyboard_handler raw pointer to null, but leak the LayoutContext
                //instance. Leaking context allows context usage in the callback should it fire.
                debugln!(DEBUG_ERROR, "Failed to close layout_change_event event, status: {:x?}", status);
                // Event still exists in firmware; null out handler to prevent stale access and leak context.
                unsafe {
                    (*self.layout_context).keyboard_handler = ptr::null_mut();
                }
                return Err(status);
            }
            // safe to drop layout change context.
            drop(unsafe { Box::from_raw(self.layout_context) });
            self.layout_context = ptr::null_mut();
            self.layout_change_event = ptr::null_mut();
        }
        Ok(())
    }

    // Creates the repeat key timer event used for keystroke repeat when a key is held.
    fn install_repeat_timer(&mut self) -> Result<(), efi::Status> {
        let context = RepeatTimerContext { keyboard_handler: self as *mut Self };
        let context_ptr = Box::into_raw(Box::new(context));

        let mut repeat_timer: efi::Event = ptr::null_mut();
        let status = self.boot_services.create_event(
            efi::EVT_TIMER | efi::EVT_NOTIFY_SIGNAL,
            efi::TPL_NOTIFY,
            Some(on_repeat_timer),
            context_ptr as *mut c_void,
            ptr::addr_of_mut!(repeat_timer),
        );
        if status.is_error() {
            drop(unsafe { Box::from_raw(context_ptr) });
            return Err(status);
        }

        self.repeat_timer_event = repeat_timer;
        self.repeat_context = context_ptr;
        Ok(())
    }

    // Closes the repeat timer event and frees the context.
    fn uninstall_repeat_timer(&mut self) -> Result<(), efi::Status> {
        if !self.repeat_timer_event.is_null() {
            let status = self.boot_services.set_timer(self.repeat_timer_event, efi::TIMER_CANCEL, 0);
            if status.is_error() {
                debugln!(DEBUG_ERROR, "Failed to cancel repeat_timer, status: {:x?}", status);
                // Timer may still fire; null out handler to prevent stale access and leak context.
                unsafe {
                    (*self.repeat_context).keyboard_handler = ptr::null_mut();
                }
                return Err(status);
            }
            self.repeat_key = None;

            let status = self.boot_services.close_event(self.repeat_timer_event);
            if status.is_error() {
                debugln!(DEBUG_ERROR, "Failed to close repeat_timer event, status: {:x?}", status);
                // Event still exists in firmware; null out handler to prevent stale access and leak context.
                unsafe {
                    (*self.repeat_context).keyboard_handler = ptr::null_mut();
                }
                return Err(status);
            }
            drop(unsafe { Box::from_raw(self.repeat_context) });
            self.repeat_context = ptr::null_mut();
            self.repeat_timer_event = ptr::null_mut();
        }
        Ok(())
    }

    // Installs a default keyboard layout.
    fn install_default_layout(&mut self) -> Result<(), efi::Status> {
        let mut hii_database_protocol_ptr: *mut protocols::hii_database::Protocol = ptr::null_mut();

        let status = self.boot_services.locate_protocol(
            &protocols::hii_database::PROTOCOL_GUID as *const efi::Guid as *mut efi::Guid,
            ptr::null_mut(),
            ptr::addr_of_mut!(hii_database_protocol_ptr) as *mut *mut c_void,
        );
        if status.is_error() {
            debugln!(
                DEBUG_ERROR,
                "keyboard::install_default_layout: Could not locate hii_database protocol to install keyboard layout: {:x?}",
                status
            );
            Err(status)?;
        }

        let hii_database_protocol = unsafe {
            hii_database_protocol_ptr.as_mut().expect("Bad pointer returned from successful locate protocol.")
        };

        let mut hii_handle: hii::Handle = ptr::null_mut();
        // SAFETY: `hii_database_protocol_ptr` points to a valid HII Database Protocol obtained from a
        // successful `locate_protocol` call, and the keyboard package list buffer is valid.
        let status = unsafe {
            (hii_database_protocol.new_package_list)(
                hii_database_protocol_ptr,
                hii_keyboard_layout::get_default_keyboard_pkg_list_buffer().as_ptr() as *const hii::PackageListHeader,
                ptr::null_mut(),
                ptr::addr_of_mut!(hii_handle),
            )
        };

        if status.is_error() {
            debugln!(
                DEBUG_ERROR,
                "keyboard::install_default_layout: Failed to install keyboard layout package: {:x?}",
                status
            );
            Err(status)?;
        }

        // SAFETY: `hii_database_protocol_ptr` points to a valid HII Database Protocol obtained from a
        // successful `locate_protocol` call.
        let status = unsafe {
            (hii_database_protocol.set_keyboard_layout)(
                hii_database_protocol_ptr,
                &hii_keyboard_layout::DEFAULT_KEYBOARD_LAYOUT_GUID as *const efi::Guid as *mut efi::Guid,
            )
        };
        if status.is_error() {
            debugln!(DEBUG_ERROR, "keyboard::install_default_layout: Failed to set keyboard layout: {:x?}", status);
            Err(status)?;
        }

        Ok(())
    }

    // Initializes the keyboard layout. If no layout is already installed in the system, a default layout is installed.
    fn initialize_keyboard_layout(&mut self) -> Result<(), efi::Status> {
        self.install_layout_change_event()?;

        //fake signal event to pick up any existing layout
        on_layout_update(self.layout_change_event, self.layout_context as *mut c_void);

        //install a default layout if no layout is installed.
        if self.key_queue.layout().is_none() {
            self.install_default_layout()?;
        }
        Ok(())
    }

    // Called to send HID reports to the device.
    fn send_output_reports(
        &mut self,
        hid_io: &dyn HidIo,
        output_reports: Vec<(Option<ReportId>, Vec<u8>)>,
    ) -> Result<(), efi::Status> {
        for (id, output_report) in output_reports {
            let result = hid_io.set_output_report(id.map(|x| u32::from(x) as u8), &output_report);
            if let Err(result) = result {
                debugln!(
                    DEBUG_ERROR,
                    "KeyboardHidHandler:set_output_report: unexpected error sending output report: {:?}",
                    result
                );
                return Err(result);
            }
        }
        Ok(())
    }

    /// Resets the keyboard driver state. Clears any pending key state. `extended verification` will also reset toggle
    /// state.
    pub fn reset(&mut self, extended_verification: bool) -> Result<(), efi::Status> {
        self.last_keys.clear();
        self.current_keys.clear();
        self.key_queue.reset(extended_verification);
        if extended_verification {
            self.led_state.clear();
        }
        // Cancel any active repeat timer.
        if !self.repeat_timer_event.is_null() {
            let status = self.boot_services.set_timer(self.repeat_timer_event, efi::TIMER_CANCEL, 0);
            if status.is_error() {
                debugln!(DEBUG_ERROR, "Failed to cancel repeat_timer during reset, status: {:x?}", status);
            }
        }
        self.repeat_key = None;
        Ok(())
    }

    /// Called to send LED state to the device if there has been a change in LEDs.
    pub fn update_leds(&mut self, hid_io: &dyn HidIo) -> Result<(), efi::Status> {
        let current_leds: BTreeSet<Usage> = self.key_queue.active_leds().iter().cloned().collect();
        if current_leds != self.led_state {
            self.send_led_reports(hid_io)?;
        }
        Ok(())
    }

    /// Called to send LED state to the device.
    pub fn send_led_reports(&mut self, hid_io: &dyn HidIo) -> Result<(), efi::Status> {
        let output_reports = self.generate_led_output_reports();
        self.send_output_reports(hid_io, output_reports).inspect_err(|&err| {
            debugln!(DEBUG_ERROR, "unexpected error sending output report: {:?}", err);
        })
    }

    /// Returns a clone of the keystroke at the front of the keystroke queue.
    pub fn peek_key(&mut self) -> Option<protocols::simple_text_input_ex::KeyData> {
        self.key_queue.peek_key()
    }

    /// Removes and returns the keystroke at the front of the keystroke queue.
    pub fn pop_key(&mut self) -> Option<protocols::simple_text_input_ex::KeyData> {
        self.key_queue.pop_key()
    }

    /// Returns the current key state (i.e. the SHIFT and TOGGLE state).
    pub fn get_key_state(&mut self) -> protocols::simple_text_input_ex::KeyState {
        self.key_queue.init_key_state()
    }

    /// Sets the current key state.
    pub fn set_key_toggle_state(&mut self, toggle_state: u8) {
        self.key_queue.set_key_toggle_state(toggle_state);
    }

    /// Registers a new key notify callback function to be invoked on the specified `key_data` press.
    ///
    /// Returns a handle that is used to unregister the callback if desired.
    pub fn insert_key_notify_callback(
        &mut self,
        key_data: protocols::simple_text_input_ex::KeyData,
        key_notification_function: protocols::simple_text_input_ex::KeyNotifyFunction,
    ) -> usize {
        let key_data = OrdKeyData(key_data);
        for (handle, entry) in &self.notification_callbacks {
            if entry.0 == key_data && ptr::fn_addr_eq(entry.1, key_notification_function) {
                //this callback already exists for this key, so return the current handle.
                return *handle;
            }
        }
        // key_data/callback combo doesn't exist, create a new registration for it.
        self.next_notify_handle += 1;
        self.notification_callbacks.insert(self.next_notify_handle, (key_data.clone(), key_notification_function));
        self.key_queue.add_notify_key(key_data);
        self.next_notify_handle
    }

    /// Unregisters a previously registered key notify callback function.
    pub fn remove_key_notify_callback(&mut self, notification_handle: usize) -> Result<(), efi::Status> {
        if let Some(entry) = self.notification_callbacks.remove(&notification_handle) {
            let removed_key = entry.0;
            if !self.notification_callbacks.values().any(|(key, _)| *key == removed_key) {
                // no other handlers exist for the key in the removed entry, so remove it from the key_queue as well.
                self.key_queue.remove_notify_key(&removed_key);
            }
            Ok(())
        } else {
            Err(efi::Status::INVALID_PARAMETER)
        }
    }

    /// Returns the set of keys that have pending callbacks, along with the vector of callback functions associated with
    /// each key.
    pub fn pending_callbacks(
        &mut self,
    ) -> (Option<protocols::simple_text_input_ex::KeyData>, Vec<protocols::simple_text_input_ex::KeyNotifyFunction>)
    {
        if let Some(pending_notify_key) = self.key_queue.pop_notify_key() {
            let mut pending_callbacks = Vec::new();
            for (key, callback) in self.notification_callbacks.values() {
                if OrdKeyData(pending_notify_key).matches_registered_key(key) {
                    pending_callbacks.push(*callback);
                }
            }
            (Some(pending_notify_key), pending_callbacks)
        } else {
            (None, Vec::new())
        }
    }

    /// Returns the agent associated with this KeyboardHidHandler
    pub fn agent(&self) -> efi::Handle {
        self.agent
    }

    /// Returns the controller associated with this KeyboardHidHandler.
    pub fn controller(&self) -> Option<efi::Handle> {
        self.controller
    }

    #[cfg(test)]
    pub fn set_controller(&mut self, controller: Option<efi::Handle>) {
        self.controller = controller;
    }

    #[cfg(test)]
    pub fn set_layout(&mut self, layout: Option<hii_keyboard_layout::HiiKeyboardLayout>) {
        self.key_queue.set_layout(layout)
    }
    #[cfg(test)]
    pub fn set_notify_event(&mut self, event: efi::Event) {
        self.key_notify_event = event;
    }
}

// Notification function called when Ctrl-Alt-Delete is pressed.
extern "efiapi" fn reset_notification_function(key_data: *mut protocols::simple_text_input_ex::KeyData) -> efi::Status {
    // Any DEL key press will trigger this callback; check that it is qualified with a CTRL-ALT state.
    // This is done here to allow for easier checking for arbitrary CTRL-ALT presses (left or right) rather than
    // registering a separate callback for each possible combination.

    if key_data.is_null() {
        return efi::Status::INVALID_PARAMETER;
    }

    // SAFETY: null-checked above, using read_unaligned to avoid any alignment issues.
    let key_data = unsafe { key_data.read_unaligned() };
    if key_data.key.scan_code != key_queue::SCAN_DELETE {
        return efi::Status::SUCCESS; // No delete
    }

    // Check that DEL is qualified with valid CTRL-ALT state.
    if key_data.key_state.key_shift_state & SHIFT_STATE_VALID == 0
        || key_data.key_state.key_shift_state & (LEFT_CONTROL_PRESSED | RIGHT_CONTROL_PRESSED) == 0
        || key_data.key_state.key_shift_state & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED) == 0
    {
        // DEL pressed without valid CTRL-ALT qualification, so do not reset.
        return efi::Status::SUCCESS;
    }

    //DEL scan code received with shift state indicating CTRL-ALT also pressed.
    debugln!(DEBUG_WARN, "Ctrl-Alt-Del pressed, resetting system.");
    if let Some(runtime_services) = unsafe { RUNTIME_SERVICES.load(Ordering::SeqCst).as_ref() } {
        // SAFETY: `runtime_services` is a valid Runtime Services table loaded from the global pointer.
        unsafe {
            (runtime_services.reset_system)(efi::RESET_COLD, efi::Status::SUCCESS, 0, core::ptr::null_mut());
        }
    }
    panic!("Reset failed.");
}

impl HidReportReceiver for KeyboardHidHandler {
    fn initialize(&mut self, controller: efi::Handle, hid_io: &dyn HidIo) -> Result<(), efi::Status> {
        let descriptor = hid_io.get_report_descriptor()?;
        self.process_descriptor(descriptor)?;
        self.reset(true)?;
        self.install_protocol_interfaces(controller)?;
        self.initialize_keyboard_layout()?;
        self.install_repeat_timer()?;

        // Register a Ctrl-Alt-Delete handler to reset the system. Register only for DEL scan code; CTRL-ALT
        // shift state will be qualified in the callback.
        let reset_key_data = protocols::simple_text_input_ex::KeyData {
            key: protocols::simple_text_input::InputKey { scan_code: key_queue::SCAN_DELETE, unicode_char: 0 },
            key_state: protocols::simple_text_input_ex::KeyState { key_toggle_state: 0, key_shift_state: 0 },
        };

        let _ = self.insert_key_notify_callback(reset_key_data, reset_notification_function);

        Ok(())
    }

    fn receive_report(&mut self, report: &[u8], hid_io: &dyn HidIo) {
        let old_tpl = self.boot_services.raise_tpl(efi::TPL_NOTIFY);

        let mut output_reports = Vec::new();
        'report_processing: {
            if report.is_empty() {
                break 'report_processing;
            }
            // determine whether report includes report id byte and adjust the buffer as needed.
            let (report_id, report) = match self.report_id_present {
                true => (Some(ReportId::from(&report[0..1])), &report[1..]),
                false => (None, &report[0..]),
            };

            if report.is_empty() {
                break 'report_processing;
            }

            if let Some(report_data) = self.input_reports.get(&report_id).cloned() {
                if report.len() != report_data.report_size {
                    //Some devices report extra bytes in their reports. Warn about this, but try and process anyway.
                    debugln!(
                        DEBUG_VERBOSE,
                        "{:?}:{:?} unexpected report length for report_id: {:?}. expected {:?}, actual {:?}",
                        function!(),
                        line!(),
                        report_id,
                        report_data.report_size,
                        report.len()
                    );
                    debugln!(DEBUG_VERBOSE, "report: {:x?}", report);
                }

                //reset currently active keys to empty set.
                self.current_keys.clear();

                // hand the report data to the handler for each relevant field for field-specific processing.
                for field in report_data.relevant_variable_fields {
                    (field.report_handler)(self, field.field, report);
                }

                for field in report_data.relevant_array_fields {
                    (field.report_handler)(self, field.field, report);
                }

                //check if any key state has changed.
                if self.last_keys != self.current_keys {
                    // process keys that are not in both sets: that is the set of keys that have changed.
                    // XOR on the sets yields a set of keys that are in either last or current keys, but not both.

                    // Modifier keys need to be processed first so that normal key processing includes modifiers that showed up in
                    // the same report. The key sets are sorted by Usage, and modifier keys all have higher usages than normal keys
                    // - so use a reverse iterator to process the modifier keys first. In addition, all released keys should be
                    // processed first so that pressed keys (which typically generate key stroke events) have the most recent
                    // key state associated with them.
                    let mut released_keys = Vec::new();
                    let mut pressed_keys = Vec::new();
                    for changed_key in (&self.last_keys ^ &self.current_keys).into_iter().rev() {
                        if self.last_keys.contains(&changed_key) {
                            //In the last key list, but not in current. This is a key release.
                            released_keys.push(changed_key);
                        } else {
                            //Not in last, so must be in current. This is a key press.
                            pressed_keys.push(changed_key);
                        }
                    }
                    for key in &released_keys {
                        self.key_queue.keystroke(*key, key_queue::KeyAction::KeyUp);
                    }

                    // If the released key was the repeat key, cancel repeat and pick a new
                    // candidate from the remaining held keys.
                    if let Some(repeat_usage) = self.repeat_key
                        && released_keys.contains(&repeat_usage)
                    {
                        self.boot_services.set_timer(self.repeat_timer_event, efi::TIMER_CANCEL, 0);
                        self.repeat_key = None;

                        // Find a repeatable key among the still-held keys.
                        let new_candidate =
                            self.current_keys.iter().rev().find(|k| self.key_queue.is_repeatable_key(**k)).copied();
                        if let Some(usage) = new_candidate {
                            self.repeat_key = Some(usage);
                            self.boot_services.set_timer(
                                self.repeat_timer_event,
                                efi::TIMER_RELATIVE,
                                REPEAT_KEY_DELAY,
                            );
                        }
                    }

                    // Track the last newly pressed repeatable key to set as the repeat candidate.
                    let mut new_repeat_key: Option<Usage> = None;
                    for key in &pressed_keys {
                        self.key_queue.keystroke(*key, key_queue::KeyAction::KeyDown);
                        if self.key_queue.is_repeatable_key(*key) {
                            new_repeat_key = Some(*key);
                        }
                    }

                    // If a new repeatable key was pressed, arm the repeat delay timer.
                    if let Some(usage) = new_repeat_key {
                        self.repeat_key = Some(usage);
                        self.boot_services.set_timer(self.repeat_timer_event, efi::TIMER_RELATIVE, REPEAT_KEY_DELAY);
                    }

                    //after processing all the key strokes, check if any keys were pressed that should trigger the notifier callback
                    //and if so, signal the event to trigger notify processing at the appropriate TPL.
                    if self.key_queue.peek_notify_key().is_some() {
                        self.boot_services.signal_event(self.key_notify_event);
                    }

                    //after processing all the key strokes, send updated LED state if required.
                    output_reports = self.generate_led_output_reports();
                }
                //after all key handling is complete for this report, update the last key set to match the current key set.
                self.last_keys = self.current_keys.clone();
            }
        }

        self.boot_services.restore_tpl(old_tpl);

        // if any output reports, send them after releasing handler.
        if let Err(result) = self.send_output_reports(hid_io, output_reports) {
            debugln!(DEBUG_ERROR, "unexpected error sending output report: {:?}", result);
        }
    }
}

impl Drop for KeyboardHidHandler {
    fn drop(&mut self) {
        // Close repeat timer first — its callback may reference protocol events.
        if let Err(status) = self.uninstall_repeat_timer() {
            debugln!(DEBUG_ERROR, "KeyboardHidHandler::drop: Failed to close repeat_timer: {:?}", status);
        }
        if let Some(controller) = self.controller {
            if let Err(status) = simple_text_in::SimpleTextInFfi::uninstall(self.boot_services, self.agent, controller)
            {
                debugln!(DEBUG_ERROR, "KeyboardHidHandler::drop: Failed to uninstall simple_text_in: {:?}", status);
            }
            if let Err(status) =
                simple_text_in_ex::SimpleTextInExFfi::uninstall(self.boot_services, self.agent, controller)
            {
                debugln!(DEBUG_ERROR, "KeyboardHidHandler::drop: Failed to uninstall simple_text_in: {:?}", status);
            }
        }
        if let Err(status) = self.uninstall_layout_change_event() {
            debugln!(DEBUG_ERROR, "KeyboardHidHandler::drop: Failed to close layout_change_event: {:?}", status);
        }
    }
}

// Handles the repeat timer event. When a repeatable key is held, this fires after the initial delay (and then at the
// repeat rate) to re-enqueue the held key into the key queue. Runs at TPL_NOTIFY for mutual exclusion with other
// keyboard handler access.
extern "efiapi" fn on_repeat_timer(_event: efi::Event, context: *mut c_void) {
    let Some(context) = (unsafe { (context as *mut RepeatTimerContext).as_mut() }) else {
        debugln!(DEBUG_ERROR, "on_repeat_timer invoked with null context pointer");
        return;
    };

    let Some(keyboard_handler) = (unsafe { context.keyboard_handler.as_mut() }) else {
        debugln!(DEBUG_ERROR, "on_repeat_timer invoked with invalid handler");
        return;
    };

    let Some(repeat_usage) = keyboard_handler.repeat_key else {
        return;
    };

    // Re-process the held key as a new KeyDown event. This picks up the current modifier/toggle state.
    keyboard_handler.key_queue.keystroke(repeat_usage, key_queue::KeyAction::KeyDown);

    // Signal key notify event if the repeated keystroke matched a registered callback.
    if keyboard_handler.key_queue.peek_notify_key().is_some() {
        keyboard_handler.boot_services.signal_event(keyboard_handler.key_notify_event);
    }

    // Re-arm the timer at the repeat rate for the next repeat.
    keyboard_handler.boot_services.set_timer(keyboard_handler.repeat_timer_event, efi::TIMER_RELATIVE, REPEAT_KEY_RATE);
}

// handles keyboard layout change event that occurs when a new keyboard layout is set.
extern "efiapi" fn on_layout_update(_event: efi::Event, context: *mut c_void) {
    let context = unsafe { (context as *mut LayoutChangeContext).as_mut() }.expect("bad context pointer");
    let old_tpl = context.boot_services.raise_tpl(efi::TPL_NOTIFY);

    'layout_processing: {
        if context.keyboard_handler.is_null() {
            debugln!(DEBUG_ERROR, "on_layout_update invoked with invalid handler");
            break 'layout_processing;
        }

        let keyboard_handler = unsafe { context.keyboard_handler.as_mut() }.expect("bad keyboard handler");

        let mut hii_database_protocol_ptr: *mut protocols::hii_database::Protocol = ptr::null_mut();
        let status = context.boot_services.locate_protocol(
            &protocols::hii_database::PROTOCOL_GUID as *const efi::Guid as *mut efi::Guid,
            ptr::null_mut(),
            ptr::addr_of_mut!(hii_database_protocol_ptr) as *mut *mut c_void,
        );

        if status.is_error() {
            //nothing to do if there is no hii protocol.
            break 'layout_processing;
        }

        let hii_database_protocol = unsafe {
            hii_database_protocol_ptr.as_mut().expect("Bad pointer returned from successful locate protocol.")
        };

        // retrieve keyboard layout size
        let mut layout_buffer_len: u16 = 0;
        // SAFETY: `hii_database_protocol_ptr` points to a valid HII Database Protocol obtained from a
        // successful `locate_protocol` call; `layout_buffer_len` is a valid local.
        match unsafe {
            (hii_database_protocol.get_keyboard_layout)(
                hii_database_protocol_ptr,
                ptr::null_mut(),
                &mut layout_buffer_len as *mut u16,
                ptr::null_mut(),
            )
        } {
            efi::Status::NOT_FOUND => break 'layout_processing,
            status if status != efi::Status::BUFFER_TOO_SMALL => {
                debugln!(
                    DEBUG_ERROR,
                    "{:}: unexpected return from get_keyboard_layout when trying to determine length: {:x?}",
                    function!(),
                    status
                );
                break 'layout_processing;
            }
            _ => (),
        }

        let mut keyboard_layout_buffer = vec![0u8; layout_buffer_len as usize];
        // SAFETY: `hii_database_protocol_ptr` points to a valid HII Database Protocol obtained from a
        // successful `locate_protocol` call; `keyboard_layout_buffer` is sized per the prior length query.
        let status = unsafe {
            (hii_database_protocol.get_keyboard_layout)(
                hii_database_protocol_ptr,
                ptr::null_mut(),
                &mut layout_buffer_len as *mut u16,
                keyboard_layout_buffer.as_mut_ptr() as *mut protocols::hii_database::KeyboardLayout<0>,
            )
        };

        if status.is_error() {
            debugln!(DEBUG_ERROR, "Unexpected return from get_keyboard_layout: {:x?}", status);
            break 'layout_processing;
        }

        let keyboard_layout = hii_keyboard_layout::keyboard_layout_from_buffer(&keyboard_layout_buffer);
        match keyboard_layout {
            Ok(keyboard_layout) => {
                keyboard_handler.key_queue.set_layout(Some(keyboard_layout));
            }
            Err(_) => {
                debugln!(DEBUG_ERROR, "keyboard::on_layout_update: Could not parse keyboard layout buffer.");
                break 'layout_processing;
            }
        }
    }

    context.boot_services.restore_tpl(old_tpl);
}

#[cfg(test)]
mod test {

    use core::{ffi::c_void, mem::MaybeUninit, ptr, slice::from_raw_parts_mut};

    use hii_keyboard_layout::HiiKeyboardLayout;
    use mu_rust_helpers::function;
    use r_efi::{efi, hii, protocols};
    use scroll::Pwrite;
    use std::sync::Mutex;

    use crate::{
        boot_services::MockUefiBootServices,
        hid_io::{HidReportReceiver, MockHidIo},
        keyboard::{KeyboardHidHandler, LayoutChangeContext, key_queue::OrdKeyData, on_layout_update},
    };

    static BOOT_KEYBOARD_REPORT_DESCRIPTOR: &[u8] = &[
        0x05, 0x01, // USAGE_PAGE (Generic Desktop)
        0x09, 0x06, // USAGE (Keyboard)
        0xa1, 0x01, // COLLECTION (Application)
        0x75, 0x01, //    REPORT_SIZE (1)
        0x95, 0x08, //    REPORT_COUNT (8)
        0x05, 0x07, //    USAGE_PAGE (Key Codes)
        0x19, 0xE0, //    USAGE_MINIMUM (224)
        0x29, 0xE7, //    USAGE_MAXIMUM (231)
        0x15, 0x00, //    LOGICAL_MAXIMUM (0)
        0x25, 0x01, //    LOGICAL_MINIMUM (1)
        0x81, 0x02, //    INPUT (Data, Var, Abs) (Modifier Byte)
        0x95, 0x01, //    REPORT_COUNT (1)
        0x75, 0x08, //    REPORT_SIZE (8)
        0x81, 0x03, //    INPUT (Const) (Reserved Byte)
        0x95, 0x05, //    REPORT_COUNT (5)
        0x75, 0x01, //    REPORT_SIZE (1)
        0x05, 0x08, //    USAGE_PAGE (LEDs)
        0x19, 0x01, //    USAGE_MINIMUM (1)
        0x29, 0x05, //    USAGE_MAXIMUM (5)
        0x91, 0x02, //    OUTPUT (Data, Var, Abs) (LED report)
        0x95, 0x01, //    REPORT_COUNT (1)
        0x75, 0x03, //    REPORT_SIZE (3)
        0x91, 0x02, //    OUTPUT (Constant) (LED report padding)
        0x95, 0x06, //    REPORT_COUNT (6)
        0x75, 0x08, //    REPORT_SIZE (8)
        0x15, 0x00, //    LOGICAL_MINIMUM (0)
        0x26, 0xff, 00, //    LOGICAL_MAXIMUM (255)
        0x05, 0x07, //    USAGE_PAGE (Key Codes)
        0x19, 0x00, //    USAGE_MINIMUM (0)
        0x2a, 0xff, 00, //    USAGE_MAXIMUM (255)
        0x81, 0x00, //    INPUT (Data, Array)
        0xc0, // END_COLLECTION
    ];

    static MOUSE_REPORT_DESCRIPTOR: &[u8] = &[
        0x05, 0x01, // USAGE_PAGE (Generic Desktop)
        0x09, 0x02, // USAGE (Mouse)
        0xa1, 0x01, // COLLECTION (Application)
        0x09, 0x01, //   USAGE(Pointer)
        0xa1, 0x00, //   COLLECTION (Physical)
        0x05, 0x09, //     USAGE_PAGE (Button)
        0x19, 0x01, //     USAGE_MINIMUM(1)
        0x29, 0x05, //     USAGE_MAXIMUM(5)
        0x15, 0x00, //     LOGICAL_MINIMUM(0)
        0x25, 0x01, //     LOGICAL_MAXIMUM(1)
        0x95, 0x05, //     REPORT_COUNT(5)
        0x75, 0x01, //     REPORT_SIZE(1)
        0x81, 0x02, //     INPUT(Data, Variable, Absolute)
        0x95, 0x01, //     REPORT_COUNT(1)
        0x75, 0x03, //     REPORT_SIZE(3)
        0x81, 0x01, //     INPUT(Constant, Array, Absolute)
        0x05, 0x01, //     USAGE_PAGE (Generic Desktop)
        0x09, 0x30, //     USAGE (X)
        0x09, 0x31, //     USAGE (Y)
        0x09, 0x38, //     USAGE (Wheel)
        0x15, 0x81, //     LOGICAL_MINIMUM (-127)
        0x25, 0x7f, //     LOGICAL_MAXIMUM (127)
        0x75, 0x08, //     REPORT_SIZE (8)
        0x95, 0x03, //     REPORT_COUNT (3)
        0x81, 0x06, //     INPUT(Data, Variable, Relative)
        0xc0, //   END_COLLECTION
        0xc0, // END_COLLECTION
    ];

    // In this module, the usage model for boot_services is global static, and so &'static dyn UefiBootServices is used
    // throughout the API. For testing, each test will have a different set of expectations on the UefiBootServices mock
    // object, and the mock object itself expects to be "mut", which makes it hard to handle as a single global static.
    // Instead, raw pointers are used to simulate a MockUefiBootServices instance with 'static lifetime.
    // This object needs to outlive anything that uses it - once created, it will live until the end of the program.
    fn create_fake_static_boot_service() -> &'static mut MockUefiBootServices {
        unsafe { Box::into_raw(Box::new(MockUefiBootServices::new())).as_mut().unwrap() }
    }

    #[test]
    fn keyboard_initialize_should_fail_for_unsupported_descriptors() {
        let boot_services = create_fake_static_boot_service();
        let mut keyboard_handler = KeyboardHidHandler::new(boot_services, 1 as efi::Handle);
        let mut hid_io = MockHidIo::new();
        hid_io.expect_set_output_report().returning(|_, _| Ok(()));
        hid_io
            .expect_get_report_descriptor()
            .returning(|| Ok(hidparser::parse_report_descriptor(MOUSE_REPORT_DESCRIPTOR).unwrap()));

        assert_eq!(keyboard_handler.initialize(2 as efi::Handle, &hid_io), Err(efi::Status::UNSUPPORTED));
    }

    #[test]
    fn keyboard_initialization_should_succeed_for_supported_descriptors() {
        let boot_services = create_fake_static_boot_service();
        boot_services.expect_create_event().returning(|_, _, _, _, _| efi::Status::SUCCESS);
        boot_services.expect_create_event_ex().returning(|_, _, _, _, _, _| efi::Status::SUCCESS);
        boot_services.expect_set_timer().returning(|_, _, _| efi::Status::SUCCESS);
        boot_services.expect_install_protocol_interface().returning(|_, _, _, _| efi::Status::SUCCESS);
        boot_services.expect_locate_protocol().returning(|_, _, _| efi::Status::NOT_FOUND);
        boot_services.expect_signal_event().returning(|_| efi::Status::SUCCESS);
        boot_services.expect_open_protocol().returning(|_, _, _, _, _, _| efi::Status::NOT_FOUND);
        boot_services.expect_raise_tpl().returning(|_| efi::TPL_APPLICATION);
        boot_services.expect_restore_tpl().returning(|_| ());

        let mut keyboard_handler = KeyboardHidHandler::new(boot_services, 1 as efi::Handle);
        let mut hid_io = MockHidIo::new();
        hid_io.expect_set_output_report().returning(|_, _| Ok(()));
        hid_io
            .expect_get_report_descriptor()
            .returning(|| Ok(hidparser::parse_report_descriptor(BOOT_KEYBOARD_REPORT_DESCRIPTOR).unwrap()));

        keyboard_handler.key_queue.set_layout(Some(hii_keyboard_layout::get_default_keyboard_layout()));

        assert_eq!(keyboard_handler.initialize(2 as efi::Handle, &hid_io), Ok(()));
    }

    #[test]
    fn keyboard_should_process_input_reports() {
        let boot_services = create_fake_static_boot_service();
        boot_services.expect_create_event().returning(|_, _, _, _, _| efi::Status::SUCCESS);
        boot_services.expect_create_event_ex().returning(|_, _, _, _, _, _| efi::Status::SUCCESS);
        boot_services.expect_set_timer().returning(|_, _, _| efi::Status::SUCCESS);
        boot_services.expect_install_protocol_interface().returning(|_, _, _, _| efi::Status::SUCCESS);
        boot_services.expect_locate_protocol().returning(|_, _, _| efi::Status::NOT_FOUND);
        boot_services.expect_signal_event().returning(|_| efi::Status::SUCCESS);
        boot_services.expect_open_protocol().returning(|_, _, _, _, _, _| efi::Status::NOT_FOUND);
        boot_services.expect_raise_tpl().returning(|_| efi::TPL_APPLICATION);
        boot_services.expect_restore_tpl().returning(|_| ());

        let mut keyboard_handler = KeyboardHidHandler::new(boot_services, 1 as efi::Handle);
        let mut hid_io = MockHidIo::new();
        hid_io.expect_set_output_report().returning(|_, _| Ok(()));
        hid_io
            .expect_get_report_descriptor()
            .returning(|| Ok(hidparser::parse_report_descriptor(BOOT_KEYBOARD_REPORT_DESCRIPTOR).unwrap()));

        keyboard_handler.key_queue.set_layout(Some(hii_keyboard_layout::get_default_keyboard_layout()));
        keyboard_handler.initialize(2 as efi::Handle, &hid_io).unwrap();

        // press the 'a' key.
        let report: &[u8] = &[0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00];
        keyboard_handler.receive_report(report, &hid_io);
        assert_eq!(keyboard_handler.key_queue.pop_key().unwrap().key.unicode_char, 'a' as u16);
        assert!(keyboard_handler.key_queue.peek_key().is_none());

        // press the 'shift' key while 'a' remains pressed.
        let report: &[u8] = &[0x02, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00];
        keyboard_handler.receive_report(report, &hid_io);
        assert!(keyboard_handler.key_queue.peek_key().is_none());

        // holding the 'shift' key while releasing 'a'.
        let report: &[u8] = &[0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00];
        keyboard_handler.receive_report(report, &hid_io);
        assert!(keyboard_handler.key_queue.peek_key().is_none());

        // holding the 'shift' key while pressing 'a' again.
        let report: &[u8] = &[0x02, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00];
        keyboard_handler.receive_report(report, &hid_io);

        assert_eq!(keyboard_handler.key_queue.pop_key().unwrap().key.unicode_char, 'A' as u16);
        assert!(keyboard_handler.key_queue.peek_key().is_none());

        // release the 'shift' key, press the 'ctrl' key, continue pressing 'a', and press 'b'
        let report: &[u8] = &[0x01, 0x00, 0x04, 0x05, 0x00, 0x00, 0x00, 0x00];
        keyboard_handler.receive_report(report, &hid_io);
        let key_data = keyboard_handler.key_queue.pop_key().unwrap();
        assert_eq!(key_data.key.unicode_char, 'b' as u16);
        assert_eq!(
            key_data.key_state.key_shift_state,
            protocols::simple_text_input_ex::SHIFT_STATE_VALID | protocols::simple_text_input_ex::LEFT_CONTROL_PRESSED
        );
        assert!(keyboard_handler.key_queue.peek_key().is_none());

        // enable partial keystrokes
        keyboard_handler.key_queue.set_key_toggle_state(protocols::simple_text_input_ex::KEY_STATE_EXPOSED);

        // release the 'ctrl' key, press 'alt', continue pressing 'a' and 'b'
        let report: &[u8] = &[0x04, 0x00, 0x04, 0x05, 0x00, 0x00, 0x00, 0x00];
        keyboard_handler.receive_report(report, &hid_io);

        let key_data = keyboard_handler.key_queue.pop_key().unwrap();
        assert_eq!(key_data.key.unicode_char, 0);
        assert_eq!(
            key_data.key_state.key_shift_state,
            protocols::simple_text_input_ex::SHIFT_STATE_VALID | protocols::simple_text_input_ex::LEFT_ALT_PRESSED
        );
        assert!(keyboard_handler.key_queue.peek_key().is_none());

        // release all the keys
        let report: &[u8] = &[0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00];
        keyboard_handler.receive_report(report, &hid_io);
        assert!(keyboard_handler.key_queue.peek_key().is_none());

        // press the right logo key
        let report: &[u8] = &[0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00];
        keyboard_handler.receive_report(report, &hid_io);

        let key_data = keyboard_handler.key_queue.pop_key().unwrap();
        assert_eq!(key_data.key.unicode_char, 0);
        assert_eq!(
            key_data.key_state.key_shift_state,
            protocols::simple_text_input_ex::SHIFT_STATE_VALID | protocols::simple_text_input_ex::RIGHT_LOGO_PRESSED
        );
        assert!(keyboard_handler.key_queue.peek_key().is_none());

        // simulate rollover
        let report: &[u8] = &[0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01];
        keyboard_handler.receive_report(report, &hid_io);
        assert!(keyboard_handler.key_queue.peek_key().is_none());

        // pass some unsupported keys
        let report: &[u8] = &[0x00, 0x00, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0];
        keyboard_handler.receive_report(report, &hid_io);
        assert!(keyboard_handler.key_queue.peek_key().is_none());

        // try all possible modifiers and all possible keys and make sure it doesn't panic.
        // some of these may generate LED reports
        let mut hid_io = MockHidIo::new();
        hid_io.expect_set_output_report().returning(|_, _| Ok(()));
        for i in 0..0xff {
            // avoid sending "Delete" keys as it will cause reset if CTRL-ALT are pressed.
            if (i == 0x4C) || (i == 0x63) {
                continue;
            }
            let report: &[u8] = &[i, 0x00, i, 0x00, 0x00, 0x00, 0x00, 0x00];
            keyboard_handler.receive_report(report, &hid_io);
        }
    }

    #[test]
    fn keyboard_should_install_layout_if_not_already_present() {
        let boot_services = create_fake_static_boot_service();
        boot_services.expect_create_event().returning(|_, _, _, _, event| unsafe {
            event.write(3 as efi::Event);
            efi::Status::SUCCESS
        });
        boot_services.expect_create_event_ex().returning(|_, _, _, _, _, event| unsafe {
            event.write(4 as efi::Event);
            efi::Status::SUCCESS
        });
        boot_services.expect_install_protocol_interface().returning(|_, _, _, _| efi::Status::SUCCESS);
        boot_services.expect_signal_event().returning(|_| efi::Status::SUCCESS);
        boot_services.expect_open_protocol().returning(|_, _, _, _, _, _| efi::Status::NOT_FOUND);
        boot_services.expect_raise_tpl().returning(|_| efi::TPL_APPLICATION);
        boot_services.expect_restore_tpl().returning(|_| ());
        boot_services.expect_close_event().returning(|_| efi::Status::SUCCESS);
        boot_services.expect_set_timer().returning(|_, _, _| efi::Status::SUCCESS);

        static mut HANDLER: *mut KeyboardHidHandler = core::ptr::null_mut();

        extern "efiapi" fn new_package_list(
            _this: *const protocols::hii_database::Protocol,
            _package_list: *const hii::PackageListHeader,
            _driver_handle: efi::Handle,
            _handle: *mut hii::Handle,
        ) -> efi::Status {
            efi::Status::SUCCESS
        }

        const TEST_KEYBOARD_GUID: efi::Guid =
            efi::Guid::from_fields(0xf1796c10, 0xdafb, 0x4989, 0xa0, 0x82, &[0x75, 0xe9, 0x65, 0x76, 0xbe, 0x52]);

        static TEST_KEYBOARD_LAYOUT: Mutex<HiiKeyboardLayout> =
            Mutex::new(HiiKeyboardLayout { keys: Vec::new(), guid: TEST_KEYBOARD_GUID, descriptions: Vec::new() });

        //make a test keyboard layout that is different than the default.
        TEST_KEYBOARD_LAYOUT.lock().unwrap().keys = hii_keyboard_layout::get_default_keyboard_layout().keys.clone();
        TEST_KEYBOARD_LAYOUT.lock().unwrap().descriptions =
            hii_keyboard_layout::get_default_keyboard_layout().descriptions.clone();
        TEST_KEYBOARD_LAYOUT.lock().unwrap().guid = hii_keyboard_layout::DEFAULT_KEYBOARD_LAYOUT_GUID;

        // modify the default layout to make sure it is not the same as the default.
        TEST_KEYBOARD_LAYOUT.lock().unwrap().guid = TEST_KEYBOARD_GUID;
        TEST_KEYBOARD_LAYOUT.lock().unwrap().keys.pop();
        TEST_KEYBOARD_LAYOUT.lock().unwrap().keys.pop();
        TEST_KEYBOARD_LAYOUT.lock().unwrap().keys.pop();
        TEST_KEYBOARD_LAYOUT.lock().unwrap().descriptions[0].description = "Test Keyboard Layout".to_string();
        TEST_KEYBOARD_LAYOUT.lock().unwrap().descriptions[0].language = "ts-TS".to_string();

        extern "efiapi" fn get_keyboard_layout(
            _this: *const protocols::hii_database::Protocol,
            _key_guid: *const efi::Guid,
            keyboard_layout_length: *mut u16,
            keyboard_layout_ptr: *mut protocols::hii_database::KeyboardLayout,
        ) -> efi::Status {
            let mut keyboard_layout_buffer = vec![0u8; 4096];
            let buffer_size = keyboard_layout_buffer
                .pwrite(&unsafe { (*ptr::addr_of!(TEST_KEYBOARD_LAYOUT)).lock().unwrap().clone() }, 0)
                .unwrap();
            keyboard_layout_buffer.resize(buffer_size, 0);
            unsafe {
                if keyboard_layout_length.read() < buffer_size as u16 {
                    keyboard_layout_length.write(buffer_size as u16);
                    efi::Status::BUFFER_TOO_SMALL
                } else {
                    if keyboard_layout_ptr.is_null() {
                        panic!("bad keyboard pointer)");
                    }
                    keyboard_layout_length.write(buffer_size as u16);
                    let slice = from_raw_parts_mut(keyboard_layout_ptr as *mut u8, buffer_size);
                    slice.copy_from_slice(&keyboard_layout_buffer);
                    efi::Status::SUCCESS
                }
            }
        }

        extern "efiapi" fn set_keyboard_layout(
            _this: *const protocols::hii_database::Protocol,
            _key_guid: *mut efi::Guid,
        ) -> efi::Status {
            let boot_services = create_fake_static_boot_service();
            boot_services.expect_raise_tpl().returning(|_| efi::TPL_APPLICATION);
            boot_services.expect_restore_tpl().returning(|_| ());
            boot_services.expect_signal_event().returning(|_| efi::Status::SUCCESS);

            boot_services.expect_locate_protocol().returning(|protocol, _, interface| {
                unsafe {
                    match *protocol {
                        protocols::hii_database::PROTOCOL_GUID => {
                            let hii_database = MaybeUninit::<protocols::hii_database::Protocol>::zeroed();
                            let mut hii_database = hii_database.assume_init();
                            hii_database.get_keyboard_layout = get_keyboard_layout;
                            interface.write(Box::into_raw(Box::new(hii_database)) as *mut c_void);
                        }
                        unexpected_protocol => {
                            panic!("unexpected locate protocol request for {:?}", unexpected_protocol)
                        }
                    }
                }
                efi::Status::SUCCESS
            });

            let context = LayoutChangeContext { boot_services, keyboard_handler: unsafe { HANDLER } };
            on_layout_update(
                3 as efi::Event,
                &context as *const LayoutChangeContext as *mut LayoutChangeContext as *mut c_void,
            );
            efi::Status::SUCCESS
        }

        boot_services.expect_locate_protocol().returning(|protocol, _, interface| {
            unsafe {
                match *protocol {
                    protocols::hii_database::PROTOCOL_GUID => {
                        let hii_database = MaybeUninit::<protocols::hii_database::Protocol>::zeroed();
                        let mut hii_database = hii_database.assume_init();
                        hii_database.new_package_list = new_package_list;
                        hii_database.set_keyboard_layout = set_keyboard_layout;
                        hii_database.get_keyboard_layout = get_keyboard_layout;

                        interface.write(Box::into_raw(Box::new(hii_database)) as *mut c_void);
                    }
                    unexpected_protocol => panic!("unexpected locate protocol request for {:?}", unexpected_protocol),
                }
            }
            efi::Status::SUCCESS
        });

        let mut keyboard_handler = KeyboardHidHandler::new(boot_services, 1 as efi::Handle);

        unsafe { HANDLER = &mut keyboard_handler as *mut KeyboardHidHandler };

        let mut hid_io = MockHidIo::new();
        hid_io.expect_set_output_report().returning(|_, _| Ok(()));
        hid_io
            .expect_get_report_descriptor()
            .returning(|| Ok(hidparser::parse_report_descriptor(BOOT_KEYBOARD_REPORT_DESCRIPTOR).unwrap()));

        assert_eq!(keyboard_handler.initialize(2 as efi::Handle, &hid_io), Ok(()));
    }

    #[test]
    fn reset_should_reset_keyboard() {
        let boot_services = create_fake_static_boot_service();
        boot_services.expect_create_event().returning(|_, _, _, _, _| efi::Status::SUCCESS);
        boot_services.expect_create_event_ex().returning(|_, _, _, _, _, _| efi::Status::SUCCESS);
        boot_services.expect_set_timer().returning(|_, _, _| efi::Status::SUCCESS);
        boot_services.expect_install_protocol_interface().returning(|_, _, _, _| efi::Status::SUCCESS);
        boot_services.expect_locate_protocol().returning(|_, _, _| efi::Status::NOT_FOUND);
        boot_services.expect_signal_event().returning(|_| efi::Status::SUCCESS);
        boot_services.expect_open_protocol().returning(|_, _, _, _, _, _| efi::Status::NOT_FOUND);
        boot_services.expect_raise_tpl().returning(|_| efi::TPL_APPLICATION);
        boot_services.expect_restore_tpl().returning(|_| ());

        let mut keyboard_handler = KeyboardHidHandler::new(boot_services, 1 as efi::Handle);
        let mut hid_io = MockHidIo::new();
        hid_io
            .expect_get_report_descriptor()
            .returning(|| Ok(hidparser::parse_report_descriptor(BOOT_KEYBOARD_REPORT_DESCRIPTOR).unwrap()));

        hid_io.expect_set_output_report().returning(|_, _| Ok(()));

        keyboard_handler.key_queue.set_layout(Some(hii_keyboard_layout::get_default_keyboard_layout()));

        assert_eq!(keyboard_handler.initialize(2 as efi::Handle, &hid_io), Ok(()));

        //buffer CapsLock + a, b, c
        let report: &[u8] = &[0x00, 0x00, 0x39, 0x04, 0x05, 0x06, 0x00, 0x00];
        keyboard_handler.receive_report(report, &hid_io);
        assert!(keyboard_handler.key_queue.peek_key().is_some());
        assert!(!keyboard_handler.led_state.is_empty());
        let prev_led_state = keyboard_handler.led_state.clone();
        assert!(!keyboard_handler.last_keys.is_empty());

        keyboard_handler.reset(false).unwrap();
        assert!(keyboard_handler.key_queue.peek_key().is_none());
        assert!(keyboard_handler.last_keys.is_empty());
        assert_eq!(keyboard_handler.led_state, prev_led_state);

        let mut hid_io = MockHidIo::new();
        hid_io.expect_set_output_report().returning(|_, report| {
            assert_eq!(report, &[0]);
            Ok(())
        });

        keyboard_handler.reset(true).unwrap();
        assert!(keyboard_handler.led_state.is_empty());
    }

    #[test]
    fn misc_functions_test() {
        let boot_services = create_fake_static_boot_service();
        boot_services.expect_create_event().returning(|_, _, _, _, _| efi::Status::SUCCESS);
        boot_services.expect_create_event_ex().returning(|_, _, _, _, _, _| efi::Status::SUCCESS);
        boot_services.expect_set_timer().returning(|_, _, _| efi::Status::SUCCESS);
        boot_services.expect_install_protocol_interface().returning(|_, _, _, _| efi::Status::SUCCESS);
        boot_services.expect_locate_protocol().returning(|_, _, _| efi::Status::NOT_FOUND);
        boot_services.expect_signal_event().returning(|_| efi::Status::SUCCESS);
        boot_services.expect_open_protocol().returning(|_, _, _, _, _, _| efi::Status::NOT_FOUND);
        boot_services.expect_raise_tpl().returning(|_| efi::TPL_APPLICATION);
        boot_services.expect_restore_tpl().returning(|_| ());

        let mut keyboard_handler = KeyboardHidHandler::new(boot_services, 1 as efi::Handle);
        let mut hid_io = MockHidIo::new();
        hid_io.expect_set_output_report().returning(|_, _| Ok(()));
        hid_io
            .expect_get_report_descriptor()
            .returning(|| Ok(hidparser::parse_report_descriptor(BOOT_KEYBOARD_REPORT_DESCRIPTOR).unwrap()));

        keyboard_handler.key_queue.set_layout(Some(hii_keyboard_layout::get_default_keyboard_layout()));
        keyboard_handler.initialize(2 as efi::Handle, &hid_io).unwrap();

        // press the 'a' key.
        let report: &[u8] = &[0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00];
        keyboard_handler.receive_report(report, &hid_io);

        assert_eq!(
            OrdKeyData(keyboard_handler.peek_key().unwrap()),
            OrdKeyData(keyboard_handler.key_queue.peek_key().unwrap())
        );
        assert_eq!(
            OrdKeyData(keyboard_handler.key_queue.peek_key().unwrap()),
            OrdKeyData(keyboard_handler.pop_key().unwrap()),
        );

        let mut hid_io = MockHidIo::new();
        hid_io.expect_set_output_report().returning(|_, _| Ok(()));

        //press capslock and release 'a' key
        let report: &[u8] = &[0x00, 0x00, 0x39, 0x00, 0x00, 0x00, 0x00, 0x00];
        keyboard_handler.receive_report(report, &hid_io);

        assert_eq!(
            keyboard_handler.get_key_state().key_shift_state,
            keyboard_handler.key_queue.init_key_state().key_shift_state
        );
        assert_eq!(
            keyboard_handler.get_key_state().key_toggle_state,
            keyboard_handler.key_queue.init_key_state().key_toggle_state
        );

        keyboard_handler.set_key_toggle_state(
            protocols::simple_text_input_ex::KEY_STATE_EXPOSED | protocols::simple_text_input_ex::CAPS_LOCK_ACTIVE,
        );
        assert_eq!(
            keyboard_handler.get_key_state().key_toggle_state,
            protocols::simple_text_input_ex::KEY_STATE_EXPOSED
                | protocols::simple_text_input_ex::TOGGLE_STATE_VALID
                | protocols::simple_text_input_ex::CAPS_LOCK_ACTIVE
        );

        assert_eq!(keyboard_handler.controller(), keyboard_handler.controller);
        assert_eq!(keyboard_handler.agent(), keyboard_handler.agent);
    }

    #[test]
    fn insert_and_remove_key_notifies_should_update_key_notify_structures() {
        let boot_services = create_fake_static_boot_service();
        boot_services.expect_create_event().returning(|_, _, _, _, _| efi::Status::SUCCESS);
        boot_services.expect_create_event_ex().returning(|_, _, _, _, _, _| efi::Status::SUCCESS);
        boot_services.expect_set_timer().returning(|_, _, _| efi::Status::SUCCESS);
        boot_services.expect_install_protocol_interface().returning(|_, _, _, _| efi::Status::SUCCESS);
        boot_services.expect_locate_protocol().returning(|_, _, _| efi::Status::NOT_FOUND);
        boot_services.expect_signal_event().returning(|_| efi::Status::SUCCESS);
        boot_services.expect_open_protocol().returning(|_, _, _, _, _, _| efi::Status::NOT_FOUND);
        boot_services.expect_raise_tpl().returning(|_| efi::TPL_APPLICATION);
        boot_services.expect_restore_tpl().returning(|_| ());

        let mut keyboard_handler = KeyboardHidHandler::new(boot_services, 1 as efi::Handle);
        let mut hid_io = MockHidIo::new();
        hid_io.expect_set_output_report().returning(|_, _| Ok(()));
        hid_io
            .expect_get_report_descriptor()
            .returning(|| Ok(hidparser::parse_report_descriptor(BOOT_KEYBOARD_REPORT_DESCRIPTOR).unwrap()));

        keyboard_handler.key_queue.set_layout(Some(hii_keyboard_layout::get_default_keyboard_layout()));
        keyboard_handler.initialize(2 as efi::Handle, &hid_io).unwrap();

        extern "efiapi" fn mock_key_notify_callback(
            _key_data: *mut protocols::simple_text_input_ex::KeyData,
        ) -> efi::Status {
            // note: print here is functionally required to ensure that `mock_key_notify_callback` and
            // `mock_key_notify_callback2` are not merged into the same function by the optimizer.
            println!("{}", function!());
            efi::Status::SUCCESS
        }

        extern "efiapi" fn mock_key_notify_callback2(
            _key_data: *mut protocols::simple_text_input_ex::KeyData,
        ) -> efi::Status {
            // note: print here is functionally required to ensure that `mock_key_notify_callback` and
            // `mock_key_notify_callback2` are not merged into the same function by the optimizer.
            println!("{}", function!());
            efi::Status::SUCCESS
        }

        let mut existing_handles = Vec::new();
        let mut key_data: protocols::simple_text_input_ex::KeyData = Default::default();

        key_data.key.unicode_char = 'a' as u16;
        let handle = keyboard_handler.insert_key_notify_callback(key_data, mock_key_notify_callback);
        assert_ne!(handle, 0);
        assert!(!existing_handles.contains(&handle));
        existing_handles.push(handle);

        key_data.key.unicode_char = 'b' as u16;
        let handle = keyboard_handler.insert_key_notify_callback(key_data, mock_key_notify_callback);
        assert_ne!(handle, 0);
        assert!(!existing_handles.contains(&handle));
        existing_handles.push(handle);

        key_data.key.unicode_char = 'c' as u16;
        let handle = keyboard_handler.insert_key_notify_callback(key_data, mock_key_notify_callback);
        assert_ne!(handle, 0);
        assert!(!existing_handles.contains(&handle));
        existing_handles.push(handle);

        //insert a second callback function tied to same key
        let handle = keyboard_handler.insert_key_notify_callback(key_data, mock_key_notify_callback2);
        assert_ne!(handle, 0);
        assert!(!existing_handles.contains(&handle));
        existing_handles.push(handle);

        //insert a key_data/callback pair that is already present.
        key_data.key.unicode_char = 'a' as u16;
        let handle = keyboard_handler.insert_key_notify_callback(key_data, mock_key_notify_callback);
        assert_eq!(handle, existing_handles[0]);

        //check state after adding callbacks. Note that there is an implicit registration for reset callback, so account for that.
        assert_eq!(keyboard_handler.next_notify_handle, existing_handles.len() + 1);
        assert_eq!(keyboard_handler.notification_callbacks.len(), existing_handles.len() + 1);

        key_data.key.unicode_char = 'a' as u16;
        assert_eq!(keyboard_handler.notification_callbacks.get(&existing_handles[0]).unwrap().0, OrdKeyData(key_data));
        assert!(ptr::fn_addr_eq(
            keyboard_handler.notification_callbacks.get(&existing_handles[0]).unwrap().1,
            mock_key_notify_callback
                as extern "efiapi" fn(*mut protocols::simple_text_input_ex::KeyData) -> efi::Status
        ));
        key_data.key.unicode_char = 'b' as u16;
        assert_eq!(keyboard_handler.notification_callbacks.get(&existing_handles[1]).unwrap().0, OrdKeyData(key_data));
        assert!(ptr::fn_addr_eq(
            keyboard_handler.notification_callbacks.get(&existing_handles[1]).unwrap().1,
            mock_key_notify_callback
                as extern "efiapi" fn(*mut protocols::simple_text_input_ex::KeyData) -> efi::Status
        ));
        key_data.key.unicode_char = 'c' as u16;
        assert_eq!(keyboard_handler.notification_callbacks.get(&existing_handles[2]).unwrap().0, OrdKeyData(key_data));
        assert!(ptr::fn_addr_eq(
            keyboard_handler.notification_callbacks.get(&existing_handles[2]).unwrap().1,
            mock_key_notify_callback
                as extern "efiapi" fn(*mut protocols::simple_text_input_ex::KeyData) -> efi::Status
        ));
        assert_eq!(keyboard_handler.notification_callbacks.get(&existing_handles[3]).unwrap().0, OrdKeyData(key_data));
        assert!(ptr::fn_addr_eq(
            keyboard_handler.notification_callbacks.get(&existing_handles[3]).unwrap().1,
            mock_key_notify_callback2
                as extern "efiapi" fn(*mut protocols::simple_text_input_ex::KeyData) -> efi::Status
        ));

        //press and release 'c' key
        let report: &[u8] = &[0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00];
        keyboard_handler.receive_report(report, &hid_io);
        let report: &[u8] = &[0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00];
        keyboard_handler.receive_report(report, &hid_io);

        key_data.key_state.key_shift_state = protocols::simple_text_input_ex::SHIFT_STATE_VALID;
        key_data.key_state.key_toggle_state = protocols::simple_text_input_ex::TOGGLE_STATE_VALID;
        let (callback_key_data, callbacks) = keyboard_handler.pending_callbacks();
        assert_eq!(OrdKeyData(key_data), OrdKeyData(callback_key_data.unwrap()));
        assert!(callbacks.contains(
            &(mock_key_notify_callback
                as unsafe extern "efiapi" fn(*mut protocols::simple_text_input_ex::KeyData) -> efi::Status)
        ));
        assert!(callbacks.contains(
            &(mock_key_notify_callback2
                as unsafe extern "efiapi" fn(*mut protocols::simple_text_input_ex::KeyData) -> efi::Status)
        ));

        let (callback_key_data, callbacks) = keyboard_handler.pending_callbacks();
        assert!(callback_key_data.is_none());
        assert!(callbacks.is_empty());

        //remove one of the 'c' callbacks and make sure the other still works.
        keyboard_handler.remove_key_notify_callback(existing_handles[3]).unwrap();

        //press and release 'a' key and 'c' key
        let report: &[u8] = &[0x00, 0x00, 0x04, 0x06, 0x00, 0x00, 0x00, 0x00];
        keyboard_handler.receive_report(report, &hid_io);
        let report: &[u8] = &[0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00];
        keyboard_handler.receive_report(report, &hid_io);

        loop {
            let (callback_key_data, callbacks) = keyboard_handler.pending_callbacks();
            if let Some(callback_key_data) = callback_key_data {
                match callback_key_data.key.unicode_char {
                    char if char == 'a' as u16 || char == 'c' as u16 => {
                        assert!(callbacks.contains(
                            &(mock_key_notify_callback
                                as unsafe extern "efiapi" fn(
                                    *mut protocols::simple_text_input_ex::KeyData,
                                ) -> efi::Status)
                        ));
                    }
                    _ => panic!("unexpected pending callback key"),
                }
            } else {
                break;
            }
        }

        //remove all the callbacks.
        keyboard_handler.remove_key_notify_callback(existing_handles[0]).unwrap();
        keyboard_handler.remove_key_notify_callback(existing_handles[1]).unwrap();
        keyboard_handler.remove_key_notify_callback(existing_handles[2]).unwrap();

        //press and release 'a' key 'b' key, and 'c' key
        let report: &[u8] = &[0x00, 0x00, 0x04, 0x05, 0x06, 0x00, 0x00, 0x00];
        keyboard_handler.receive_report(report, &hid_io);
        let report: &[u8] = &[0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00];
        keyboard_handler.receive_report(report, &hid_io);

        let (callback_key_data, callbacks) = keyboard_handler.pending_callbacks();
        assert!(callback_key_data.is_none());
        assert!(callbacks.is_empty());
    }

    #[test]
    fn key_press_should_arm_repeat_timer() {
        use std::sync::atomic::{AtomicU32, AtomicU64};

        static SET_TIMER_TYPE: AtomicU32 = AtomicU32::new(0);
        static SET_TIMER_TIME: AtomicU64 = AtomicU64::new(0);

        let boot_services = create_fake_static_boot_service();
        boot_services.expect_create_event().returning(|_, _, _, _, _| efi::Status::SUCCESS);
        boot_services.expect_create_event_ex().returning(|_, _, _, _, _, _| efi::Status::SUCCESS);
        boot_services.expect_set_timer().returning(|_, timer_type, trigger_time| {
            SET_TIMER_TYPE.store(timer_type, std::sync::atomic::Ordering::SeqCst);
            SET_TIMER_TIME.store(trigger_time, std::sync::atomic::Ordering::SeqCst);
            efi::Status::SUCCESS
        });
        boot_services.expect_install_protocol_interface().returning(|_, _, _, _| efi::Status::SUCCESS);
        boot_services.expect_locate_protocol().returning(|_, _, _| efi::Status::NOT_FOUND);
        boot_services.expect_signal_event().returning(|_| efi::Status::SUCCESS);
        boot_services.expect_open_protocol().returning(|_, _, _, _, _, _| efi::Status::NOT_FOUND);
        boot_services.expect_raise_tpl().returning(|_| efi::TPL_APPLICATION);
        boot_services.expect_restore_tpl().returning(|_| ());

        let mut keyboard_handler = KeyboardHidHandler::new(boot_services, 1 as efi::Handle);
        let mut hid_io = MockHidIo::new();
        hid_io.expect_set_output_report().returning(|_, _| Ok(()));
        hid_io
            .expect_get_report_descriptor()
            .returning(|| Ok(hidparser::parse_report_descriptor(BOOT_KEYBOARD_REPORT_DESCRIPTOR).unwrap()));

        keyboard_handler.key_queue.set_layout(Some(hii_keyboard_layout::get_default_keyboard_layout()));
        keyboard_handler.initialize(2 as efi::Handle, &hid_io).unwrap();

        // press the 'a' key
        let report: &[u8] = &[0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00];
        keyboard_handler.receive_report(report, &hid_io);

        // Repeat timer should have been armed with TIMER_RELATIVE and delay
        assert_eq!(SET_TIMER_TYPE.load(std::sync::atomic::Ordering::SeqCst), efi::TIMER_RELATIVE);
        assert_eq!(SET_TIMER_TIME.load(std::sync::atomic::Ordering::SeqCst), super::REPEAT_KEY_DELAY);
        assert!(keyboard_handler.repeat_key.is_some());
    }

    #[test]
    fn key_release_should_cancel_repeat_timer() {
        use std::sync::atomic::AtomicU32;

        static LAST_TIMER_TYPE: AtomicU32 = AtomicU32::new(0xFF);

        let boot_services = create_fake_static_boot_service();
        boot_services.expect_create_event().returning(|_, _, _, _, _| efi::Status::SUCCESS);
        boot_services.expect_create_event_ex().returning(|_, _, _, _, _, _| efi::Status::SUCCESS);
        boot_services.expect_set_timer().returning(|_, timer_type, _| {
            LAST_TIMER_TYPE.store(timer_type, std::sync::atomic::Ordering::SeqCst);
            efi::Status::SUCCESS
        });
        boot_services.expect_install_protocol_interface().returning(|_, _, _, _| efi::Status::SUCCESS);
        boot_services.expect_locate_protocol().returning(|_, _, _| efi::Status::NOT_FOUND);
        boot_services.expect_signal_event().returning(|_| efi::Status::SUCCESS);
        boot_services.expect_open_protocol().returning(|_, _, _, _, _, _| efi::Status::NOT_FOUND);
        boot_services.expect_raise_tpl().returning(|_| efi::TPL_APPLICATION);
        boot_services.expect_restore_tpl().returning(|_| ());

        let mut keyboard_handler = KeyboardHidHandler::new(boot_services, 1 as efi::Handle);
        let mut hid_io = MockHidIo::new();
        hid_io.expect_set_output_report().returning(|_, _| Ok(()));
        hid_io
            .expect_get_report_descriptor()
            .returning(|| Ok(hidparser::parse_report_descriptor(BOOT_KEYBOARD_REPORT_DESCRIPTOR).unwrap()));

        keyboard_handler.key_queue.set_layout(Some(hii_keyboard_layout::get_default_keyboard_layout()));
        keyboard_handler.initialize(2 as efi::Handle, &hid_io).unwrap();

        // press the 'a' key
        let report: &[u8] = &[0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00];
        keyboard_handler.receive_report(report, &hid_io);
        assert!(keyboard_handler.repeat_key.is_some());

        // release the 'a' key
        let report: &[u8] = &[0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00];
        keyboard_handler.receive_report(report, &hid_io);

        // Timer should have been cancelled
        assert_eq!(LAST_TIMER_TYPE.load(std::sync::atomic::Ordering::SeqCst), efi::TIMER_CANCEL);
        assert!(keyboard_handler.repeat_key.is_none());
    }

    #[test]
    fn release_repeat_key_should_handoff_to_remaining_held_key() {
        let boot_services = create_fake_static_boot_service();
        boot_services.expect_create_event().returning(|_, _, _, _, _| efi::Status::SUCCESS);
        boot_services.expect_create_event_ex().returning(|_, _, _, _, _, _| efi::Status::SUCCESS);
        boot_services.expect_set_timer().returning(|_, _, _| efi::Status::SUCCESS);
        boot_services.expect_install_protocol_interface().returning(|_, _, _, _| efi::Status::SUCCESS);
        boot_services.expect_locate_protocol().returning(|_, _, _| efi::Status::NOT_FOUND);
        boot_services.expect_signal_event().returning(|_| efi::Status::SUCCESS);
        boot_services.expect_open_protocol().returning(|_, _, _, _, _, _| efi::Status::NOT_FOUND);
        boot_services.expect_raise_tpl().returning(|_| efi::TPL_APPLICATION);
        boot_services.expect_restore_tpl().returning(|_| ());

        let mut keyboard_handler = KeyboardHidHandler::new(boot_services, 1 as efi::Handle);
        let mut hid_io = MockHidIo::new();
        hid_io.expect_set_output_report().returning(|_, _| Ok(()));
        hid_io
            .expect_get_report_descriptor()
            .returning(|| Ok(hidparser::parse_report_descriptor(BOOT_KEYBOARD_REPORT_DESCRIPTOR).unwrap()));

        keyboard_handler.key_queue.set_layout(Some(hii_keyboard_layout::get_default_keyboard_layout()));
        keyboard_handler.initialize(2 as efi::Handle, &hid_io).unwrap();

        // press 'a' + 'b' simultaneously
        let report: &[u8] = &[0x00, 0x00, 0x04, 0x05, 0x00, 0x00, 0x00, 0x00];
        keyboard_handler.receive_report(report, &hid_io);
        assert!(keyboard_handler.repeat_key.is_some());

        // release 'b' (the repeat key), 'a' still held
        let report: &[u8] = &[0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00];
        keyboard_handler.receive_report(report, &hid_io);

        // 'a' should now be the repeat key
        assert_eq!(keyboard_handler.repeat_key, Some(hidparser::report_data_types::Usage::from(0x00070004u32)));
    }

    #[test]
    fn modifier_keys_should_not_trigger_repeat() {
        let boot_services = create_fake_static_boot_service();
        boot_services.expect_create_event().returning(|_, _, _, _, _| efi::Status::SUCCESS);
        boot_services.expect_create_event_ex().returning(|_, _, _, _, _, _| efi::Status::SUCCESS);
        boot_services.expect_set_timer().returning(|_, _, _| efi::Status::SUCCESS);
        boot_services.expect_install_protocol_interface().returning(|_, _, _, _| efi::Status::SUCCESS);
        boot_services.expect_locate_protocol().returning(|_, _, _| efi::Status::NOT_FOUND);
        boot_services.expect_signal_event().returning(|_| efi::Status::SUCCESS);
        boot_services.expect_open_protocol().returning(|_, _, _, _, _, _| efi::Status::NOT_FOUND);
        boot_services.expect_raise_tpl().returning(|_| efi::TPL_APPLICATION);
        boot_services.expect_restore_tpl().returning(|_| ());

        let mut keyboard_handler = KeyboardHidHandler::new(boot_services, 1 as efi::Handle);
        let mut hid_io = MockHidIo::new();
        hid_io.expect_set_output_report().returning(|_, _| Ok(()));
        hid_io
            .expect_get_report_descriptor()
            .returning(|| Ok(hidparser::parse_report_descriptor(BOOT_KEYBOARD_REPORT_DESCRIPTOR).unwrap()));

        keyboard_handler.key_queue.set_layout(Some(hii_keyboard_layout::get_default_keyboard_layout()));
        keyboard_handler.initialize(2 as efi::Handle, &hid_io).unwrap();

        // press left shift only (modifier byte bit 1)
        let report: &[u8] = &[0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00];
        keyboard_handler.receive_report(report, &hid_io);

        // Modifier keys should not trigger repeat
        assert!(keyboard_handler.repeat_key.is_none());
    }

    #[test]
    fn new_key_press_should_replace_repeat_key() {
        let boot_services = create_fake_static_boot_service();
        boot_services.expect_create_event().returning(|_, _, _, _, _| efi::Status::SUCCESS);
        boot_services.expect_create_event_ex().returning(|_, _, _, _, _, _| efi::Status::SUCCESS);
        boot_services.expect_set_timer().returning(|_, _, _| efi::Status::SUCCESS);
        boot_services.expect_install_protocol_interface().returning(|_, _, _, _| efi::Status::SUCCESS);
        boot_services.expect_locate_protocol().returning(|_, _, _| efi::Status::NOT_FOUND);
        boot_services.expect_signal_event().returning(|_| efi::Status::SUCCESS);
        boot_services.expect_open_protocol().returning(|_, _, _, _, _, _| efi::Status::NOT_FOUND);
        boot_services.expect_raise_tpl().returning(|_| efi::TPL_APPLICATION);
        boot_services.expect_restore_tpl().returning(|_| ());

        let mut keyboard_handler = KeyboardHidHandler::new(boot_services, 1 as efi::Handle);
        let mut hid_io = MockHidIo::new();
        hid_io.expect_set_output_report().returning(|_, _| Ok(()));
        hid_io
            .expect_get_report_descriptor()
            .returning(|| Ok(hidparser::parse_report_descriptor(BOOT_KEYBOARD_REPORT_DESCRIPTOR).unwrap()));

        keyboard_handler.key_queue.set_layout(Some(hii_keyboard_layout::get_default_keyboard_layout()));
        keyboard_handler.initialize(2 as efi::Handle, &hid_io).unwrap();

        // press 'a' key
        let report: &[u8] = &[0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00];
        keyboard_handler.receive_report(report, &hid_io);
        let first_repeat = keyboard_handler.repeat_key;
        assert!(first_repeat.is_some());

        // press 'a' + 'b' (add 'b' while holding 'a')
        let report: &[u8] = &[0x00, 0x00, 0x04, 0x05, 0x00, 0x00, 0x00, 0x00];
        keyboard_handler.receive_report(report, &hid_io);

        // repeat key should now be 'b' (the newly pressed key), not 'a'
        assert!(keyboard_handler.repeat_key.is_some());
        assert_ne!(keyboard_handler.repeat_key, first_repeat);
    }

    #[test]
    fn repeat_timer_callback_should_enqueue_key() {
        let boot_services = create_fake_static_boot_service();
        boot_services.expect_create_event().returning(|_, _, _, _, _| efi::Status::SUCCESS);
        boot_services.expect_create_event_ex().returning(|_, _, _, _, _, _| efi::Status::SUCCESS);
        boot_services.expect_set_timer().returning(|_, _, _| efi::Status::SUCCESS);
        boot_services.expect_install_protocol_interface().returning(|_, _, _, _| efi::Status::SUCCESS);
        boot_services.expect_locate_protocol().returning(|_, _, _| efi::Status::NOT_FOUND);
        boot_services.expect_signal_event().returning(|_| efi::Status::SUCCESS);
        boot_services.expect_open_protocol().returning(|_, _, _, _, _, _| efi::Status::NOT_FOUND);
        boot_services.expect_raise_tpl().returning(|_| efi::TPL_APPLICATION);
        boot_services.expect_restore_tpl().returning(|_| ());

        let mut keyboard_handler = KeyboardHidHandler::new(boot_services, 1 as efi::Handle);
        let mut hid_io = MockHidIo::new();
        hid_io.expect_set_output_report().returning(|_, _| Ok(()));
        hid_io
            .expect_get_report_descriptor()
            .returning(|| Ok(hidparser::parse_report_descriptor(BOOT_KEYBOARD_REPORT_DESCRIPTOR).unwrap()));

        keyboard_handler.key_queue.set_layout(Some(hii_keyboard_layout::get_default_keyboard_layout()));
        keyboard_handler.initialize(2 as efi::Handle, &hid_io).unwrap();

        // press 'a' key
        let report: &[u8] = &[0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00];
        keyboard_handler.receive_report(report, &hid_io);

        // consume the initial keystroke
        let initial_key = keyboard_handler.pop_key().unwrap();
        assert_eq!(initial_key.key.unicode_char, 'a' as u16);
        assert!(keyboard_handler.pop_key().is_none());

        // Simulate the repeat timer callback firing by calling it directly
        let context = keyboard_handler.repeat_context;
        super::on_repeat_timer(ptr::null_mut(), context as *mut c_void);

        // Should have enqueued a repeat keystroke
        let repeat_key = keyboard_handler.pop_key().unwrap();
        assert_eq!(repeat_key.key.unicode_char, 'a' as u16);
    }

    #[test]
    fn reset_should_cancel_repeat_timer() {
        use std::sync::atomic::AtomicU32;

        static LAST_TIMER_TYPE: AtomicU32 = AtomicU32::new(0xFF);

        let boot_services = create_fake_static_boot_service();
        boot_services.expect_create_event().returning(|_, _, _, _, event| unsafe {
            event.write(3 as efi::Event);
            efi::Status::SUCCESS
        });
        boot_services.expect_create_event_ex().returning(|_, _, _, _, _, _| efi::Status::SUCCESS);
        boot_services.expect_set_timer().returning(|_, timer_type, _| {
            LAST_TIMER_TYPE.store(timer_type, std::sync::atomic::Ordering::SeqCst);
            efi::Status::SUCCESS
        });
        boot_services.expect_install_protocol_interface().returning(|_, _, _, _| efi::Status::SUCCESS);
        boot_services.expect_locate_protocol().returning(|_, _, _| efi::Status::NOT_FOUND);
        boot_services.expect_signal_event().returning(|_| efi::Status::SUCCESS);
        boot_services.expect_open_protocol().returning(|_, _, _, _, _, _| efi::Status::NOT_FOUND);
        boot_services.expect_raise_tpl().returning(|_| efi::TPL_APPLICATION);
        boot_services.expect_restore_tpl().returning(|_| ());
        boot_services.expect_close_event().returning(|_| efi::Status::SUCCESS);

        let mut keyboard_handler = KeyboardHidHandler::new(boot_services, 1 as efi::Handle);
        let mut hid_io = MockHidIo::new();
        hid_io.expect_set_output_report().returning(|_, _| Ok(()));
        hid_io
            .expect_get_report_descriptor()
            .returning(|| Ok(hidparser::parse_report_descriptor(BOOT_KEYBOARD_REPORT_DESCRIPTOR).unwrap()));

        keyboard_handler.key_queue.set_layout(Some(hii_keyboard_layout::get_default_keyboard_layout()));
        keyboard_handler.initialize(2 as efi::Handle, &hid_io).unwrap();

        // press 'a' to set up repeat
        let report: &[u8] = &[0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00];
        keyboard_handler.receive_report(report, &hid_io);
        assert!(keyboard_handler.repeat_key.is_some());

        // reset should cancel repeat
        keyboard_handler.reset(false).unwrap();
        assert_eq!(LAST_TIMER_TYPE.load(std::sync::atomic::Ordering::SeqCst), efi::TIMER_CANCEL);
        assert!(keyboard_handler.repeat_key.is_none());
    }
}
