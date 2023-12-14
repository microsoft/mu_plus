//! Provides a trait abstraction for the UEFI Boot Services used in this crate.
//!
//! This module provides a trait definition to abstract the UEFI services used
//! by this crate and provides a concrete implementation of that trait that uses
//! real boot services.
//!
//! ## License
//!
//! Copyright (C) Microsoft Corporation. All rights reserved.
//!
//! SPDX-License-Identifier: BSD-2-Clause-Patent
//!
use core::{ffi::c_void, fmt::Debug, sync::atomic::AtomicPtr};
#[cfg(test)]
use mockall::automock;
use r_efi::efi;

/// Abstracts UEFI Boot Services used in this crate.
#[cfg_attr(test, automock)]
pub trait UefiBootServices {
  fn create_event(
    &self,
    r#type: u32,
    notify_tpl: efi::Tpl,
    notify_function: Option<efi::EventNotify>,
    notify_context: *mut c_void,
    event: *mut efi::Event,
  ) -> efi::Status;

  fn create_event_ex(
    &self,
    r#type: u32,
    notify_tpl: efi::Tpl,
    notify_function: Option<efi::EventNotify>,
    notify_context: *const c_void,
    event_group: *const efi::Guid,
    event: *mut efi::Event,
  ) -> efi::Status;

  fn close_event(&self, event: efi::Event) -> efi::Status;

  fn signal_event(&self, event: efi::Event) -> efi::Status;

  fn raise_tpl(&self, new_tpl: efi::Tpl) -> efi::Tpl;

  fn restore_tpl(&self, old_tpl: efi::Tpl);

  fn install_protocol_interface(
    &self,
    handle: *mut efi::Handle,
    protocol: *mut efi::Guid,
    interface_type: efi::InterfaceType,
    interface: *mut c_void,
  ) -> efi::Status;

  fn uninstall_protocol_interface(
    &self,
    handle: efi::Handle,
    protocol: *mut efi::Guid,
    interface: *mut c_void,
  ) -> efi::Status;

  fn open_protocol(
    &self,
    handle: efi::Handle,
    protocol: *mut efi::Guid,
    interface: *mut *mut c_void,
    agent_handle: efi::Handle,
    controller_handle: efi::Handle,
    attributes: u32,
  ) -> efi::Status;

  fn close_protocol(
    &self,
    handle: efi::Handle,
    protocol: *mut efi::Guid,
    agent_handle: efi::Handle,
    controller_handle: efi::Handle,
  ) -> efi::Status;

  fn locate_protocol(
    &self,
    protocol: *mut efi::Guid,
    registration: *mut c_void,
    interface: *mut *mut c_void,
  ) -> efi::Status;
}

/// Provides a concrete implementation of the [`UefiBootServices`] trait.
#[derive(Debug)]
pub struct StandardUefiBootServices {
  boot_services: AtomicPtr<efi::BootServices>,
}

impl StandardUefiBootServices {
  /// Creates a new StandardUefiBootServices.
  /// Note: attempts to use methods on this instance will panic until [`Self::initialize`] is called.
  pub const fn new() -> Self {
    Self { boot_services: AtomicPtr::new(core::ptr::null_mut()) }
  }

  /// Initializes this instance of [`StandardUefiBootServices`] with a pointer to the boot services table.
  pub fn initialize(&self, boot_services: *mut efi::BootServices) {
    self.boot_services.store(boot_services, core::sync::atomic::Ordering::SeqCst)
  }

  // Returns a reference to the boot services table. Panics if uninitialized.
  fn boot_services(&self) -> &efi::BootServices {
    let boot_services_ptr = self.boot_services.load(core::sync::atomic::Ordering::SeqCst);
    unsafe { boot_services_ptr.as_ref().expect("invalid boot_services pointer") }
  }
}

unsafe impl Sync for StandardUefiBootServices {}
unsafe impl Send for StandardUefiBootServices {}

impl UefiBootServices for StandardUefiBootServices {
  fn create_event(
    &self,
    r#type: u32,
    notify_tpl: efi::Tpl,
    notify_function: Option<efi::EventNotify>,
    notify_context: *mut c_void,
    event: *mut efi::Event,
  ) -> efi::Status {
    (self.boot_services().create_event)(r#type, notify_tpl, notify_function, notify_context, event)
  }
  fn create_event_ex(
    &self,
    r#type: u32,
    notify_tpl: efi::Tpl,
    notify_function: Option<efi::EventNotify>,
    notify_context: *const c_void,
    event_group: *const efi::Guid,
    event: *mut efi::Event,
  ) -> efi::Status {
    (self.boot_services().create_event_ex)(r#type, notify_tpl, notify_function, notify_context, event_group, event)
  }
  fn close_event(&self, event: efi::Event) -> efi::Status {
    (self.boot_services().close_event)(event)
  }
  fn signal_event(&self, event: efi::Event) -> efi::Status {
    (self.boot_services().signal_event)(event)
  }
  fn raise_tpl(&self, new_tpl: efi::Tpl) -> efi::Tpl {
    (self.boot_services().raise_tpl)(new_tpl)
  }
  fn restore_tpl(&self, old_tpl: efi::Tpl) {
    (self.boot_services().restore_tpl)(old_tpl)
  }
  fn install_protocol_interface(
    &self,
    handle: *mut efi::Handle,
    protocol: *mut efi::Guid,
    interface_type: efi::InterfaceType,
    interface: *mut c_void,
  ) -> efi::Status {
    (self.boot_services().install_protocol_interface)(handle, protocol, interface_type, interface)
  }
  fn uninstall_protocol_interface(
    &self,
    handle: efi::Handle,
    protocol: *mut efi::Guid,
    interface: *mut c_void,
  ) -> efi::Status {
    (self.boot_services().uninstall_protocol_interface)(handle, protocol, interface)
  }
  fn open_protocol(
    &self,
    handle: efi::Handle,
    protocol: *mut efi::Guid,
    interface: *mut *mut c_void,
    agent_handle: efi::Handle,
    controller_handle: efi::Handle,
    attributes: u32,
  ) -> efi::Status {
    (self.boot_services().open_protocol)(handle, protocol, interface, agent_handle, controller_handle, attributes)
  }
  fn close_protocol(
    &self,
    handle: efi::Handle,
    protocol: *mut efi::Guid,
    agent_handle: efi::Handle,
    controller_handle: efi::Handle,
  ) -> efi::Status {
    (self.boot_services().close_protocol)(handle, protocol, agent_handle, controller_handle)
  }
  fn locate_protocol(
    &self,
    protocol: *mut efi::Guid,
    registration: *mut c_void,
    interface: *mut *mut c_void,
  ) -> efi::Status {
    (self.boot_services().locate_protocol)(protocol, registration, interface)
  }
}

#[cfg(test)]
mod test {
  use core::{ffi::c_void, mem::MaybeUninit};

  use r_efi::efi;

  use super::{StandardUefiBootServices, UefiBootServices};

  extern "efiapi" fn mock_create_event(
    _type: u32,
    _notify_tpl: efi::Tpl,
    _notify_function: Option<efi::EventNotify>,
    _notify_context: *mut c_void,
    _event: *mut efi::Event,
  ) -> efi::Status {
    efi::Status::SUCCESS
  }

  extern "efiapi" fn mock_create_event_ex(
    _type: u32,
    _notify_tpl: efi::Tpl,
    _notify_function: Option<efi::EventNotify>,
    _notify_context: *const c_void,
    _event_group: *const efi::Guid,
    _event: *mut efi::Event,
  ) -> efi::Status {
    efi::Status::SUCCESS
  }
  extern "efiapi" fn mock_close_event(_event: efi::Event) -> efi::Status {
    efi::Status::SUCCESS
  }

  extern "efiapi" fn mock_signal_event(_event: efi::Event) -> efi::Status {
    efi::Status::SUCCESS
  }

  extern "efiapi" fn mock_raise_tpl(_new_tpl: efi::Tpl) -> efi::Tpl {
    efi::TPL_APPLICATION
  }

  extern "efiapi" fn mock_restore_tpl(_new_tpl: efi::Tpl) {}

  extern "efiapi" fn mock_install_protocol_interface(
    _handle: *mut efi::Handle,
    _protocol: *mut efi::Guid,
    _interface_type: efi::InterfaceType,
    _interface: *mut c_void,
  ) -> efi::Status {
    efi::Status::SUCCESS
  }

  extern "efiapi" fn mock_uninstall_protocol_interface(
    _handle: efi::Handle,
    _protocol: *mut efi::Guid,
    _interface: *mut c_void,
  ) -> efi::Status {
    efi::Status::SUCCESS
  }

  extern "efiapi" fn mock_open_protocol(
    _handle: efi::Handle,
    _protocol: *mut efi::Guid,
    _interface: *mut *mut c_void,
    _agent_handle: efi::Handle,
    _controller_handle: efi::Handle,
    _attributes: u32,
  ) -> efi::Status {
    efi::Status::SUCCESS
  }

  extern "efiapi" fn mock_close_protocol(
    _handle: efi::Handle,
    _protocol: *mut efi::Guid,
    _agent_handle: efi::Handle,
    _controller_handle: efi::Handle,
  ) -> efi::Status {
    efi::Status::SUCCESS
  }

  extern "efiapi" fn mock_locate_protocol(
    _protocol: *mut efi::Guid,
    _registration: *mut c_void,
    _interface: *mut *mut c_void,
  ) -> efi::Status {
    efi::Status::SUCCESS
  }

  #[test]
  fn standard_uefi_boot_services_should_wrap_boot_services() {
    let boot_services = MaybeUninit::<efi::BootServices>::zeroed();
    let mut boot_services = unsafe { boot_services.assume_init() };
    boot_services.create_event = mock_create_event;
    boot_services.create_event_ex = mock_create_event_ex;
    boot_services.close_event = mock_close_event;
    boot_services.signal_event = mock_signal_event;
    boot_services.raise_tpl = mock_raise_tpl;
    boot_services.restore_tpl = mock_restore_tpl;
    boot_services.install_protocol_interface = mock_install_protocol_interface;
    boot_services.uninstall_protocol_interface = mock_uninstall_protocol_interface;
    boot_services.open_protocol = mock_open_protocol;
    boot_services.close_protocol = mock_close_protocol;
    boot_services.locate_protocol = mock_locate_protocol;

    const TEST_GUID: efi::Guid = efi::Guid::from_fields(0, 0, 0, 0, 0, &[0, 0, 0, 0, 0, 0]);
    let mut event = 1 as efi::Event;
    let mut handle = 2 as efi::Handle;

    let test_boot_services = StandardUefiBootServices::new();
    test_boot_services.initialize(&mut boot_services as *mut efi::BootServices);
    assert_eq!(
      test_boot_services.create_event(0, efi::TPL_NOTIFY, None, core::ptr::null_mut(), core::ptr::addr_of_mut!(event)),
      efi::Status::SUCCESS
    );

    assert_eq!(
      test_boot_services.create_event_ex(
        0,
        efi::TPL_NOTIFY,
        None,
        core::ptr::null_mut(),
        &TEST_GUID as *const efi::Guid,
        core::ptr::addr_of_mut!(event),
      ),
      efi::Status::SUCCESS
    );

    assert_eq!(test_boot_services.close_event(event), efi::Status::SUCCESS);
    assert_eq!(test_boot_services.signal_event(event), efi::Status::SUCCESS);
    assert_eq!(test_boot_services.raise_tpl(efi::TPL_HIGH_LEVEL), efi::TPL_APPLICATION);
    test_boot_services.restore_tpl(efi::TPL_APPLICATION);
    assert_eq!(
      test_boot_services.install_protocol_interface(
        core::ptr::addr_of_mut!(handle),
        &TEST_GUID as *const efi::Guid as *mut efi::Guid,
        efi::NATIVE_INTERFACE,
        core::ptr::null_mut()
      ),
      efi::Status::SUCCESS
    );
    assert_eq!(
      test_boot_services.uninstall_protocol_interface(
        handle,
        &TEST_GUID as *const efi::Guid as *mut efi::Guid,
        core::ptr::null_mut()
      ),
      efi::Status::SUCCESS
    );
    assert_eq!(
      test_boot_services.open_protocol(
        handle,
        &TEST_GUID as *const efi::Guid as *mut efi::Guid,
        core::ptr::null_mut(),
        handle,
        handle,
        0
      ),
      efi::Status::SUCCESS
    );
    assert_eq!(
      test_boot_services.close_protocol(handle, &TEST_GUID as *const efi::Guid as *mut efi::Guid, handle, handle),
      efi::Status::SUCCESS
    );
    assert_eq!(
      test_boot_services.locate_protocol(
        &TEST_GUID as *const efi::Guid as *mut efi::Guid,
        core::ptr::null_mut(),
        core::ptr::null_mut()
      ),
      efi::Status::SUCCESS
    );
  }
}
