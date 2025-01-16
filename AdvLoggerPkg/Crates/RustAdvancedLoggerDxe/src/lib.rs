//! Rust Advanced Logger
//!
//! Rust Wrapper for the UEFI Advanced Logger protocol.
//!
//! ## Examples and Usage
//!
//! ```no_run
//! use rust_advanced_logger_dxe::{init_debug, debugln, DEBUG_INFO};
//! use r_efi::efi::Status;
//! pub extern "efiapi" fn efi_main(
//!    _image_handle: *const core::ffi::c_void,
//!    _system_table: *const r_efi::system::SystemTable,
//!  ) -> u64 {
//!
//!    //Initialize debug logging - no output without this.
//!    init_debug(unsafe { (*_system_table).boot_services});
//!
//!    debugln!(DEBUG_INFO, "Hello, World. This is {:} in {:}.", "rust", "UEFI");
//!
//!    Status::SUCCESS.as_usize() as u64
//! }
//! ```
//!
//! ## License
//!
//! Copyright (C) Microsoft Corporation. All rights reserved.
//!
//! SPDX-License-Identifier: BSD-2-Clause-Patent
//!
#![no_std]

#[cfg(any(doc, feature = "std"))]
extern crate std; //allow rustdoc links to reference std (e.g. println docs below).

use core::{
    fmt::{self, Write},
    ops::Deref,
    ptr,
    sync::atomic::{AtomicPtr, Ordering},
};
use r_efi::efi;

use boot_services::{self, protocol_handler::Protocol};

//Global static logger instance - this is a singleton.
static LOGGER: AdvancedLogger = AdvancedLogger::new();

/// Standard UEFI DEBUG_INIT level.
pub const DEBUG_INIT: usize = 0x00000001;
/// Standard UEFI DEBUG_WARN level.
pub const DEBUG_WARN: usize = 0x00000002;
/// Standard UEFI DEBUG_INFO level.
pub const DEBUG_INFO: usize = 0x00000040;
/// Standard UEFI DEBUG_VERBOSE level.
pub const DEBUG_VERBOSE: usize = 0x00400000;
/// Standard UEFI DEBUG_ERROR level.
pub const DEBUG_ERROR: usize = 0x80000000;

// AdvancedLogger protocol definition. Mirrors C definition in AdvLoggerPkg/Include/Protocol/AdvancedLogger.h
const ADVANCED_LOGGER_PROTOCOL_GUID: efi::Guid =
    efi::Guid::from_fields(0x434f695c, 0xef26, 0x4a12, 0x9e, 0xba, &[0xdd, 0xef, 0x00, 0x97, 0x49, 0x7c]);

type AdvancedLoggerWriteProtocol = extern "efiapi" fn(*const AdvancedLoggerProtocolInterface, usize, *const u8, usize);

const ADVANCED_LOGGER_PROTOCOL: AdvancedLoggerProtocol = AdvancedLoggerProtocol {};

#[repr(C)]
struct AdvancedLoggerProtocolInterface {
    signature: u32,
    version: u32,
    write_log: AdvancedLoggerWriteProtocol,
}

struct AdvancedLoggerProtocol;

unsafe impl Protocol for AdvancedLoggerProtocol {
    type Interface = AdvancedLoggerProtocolInterface;
    fn protocol_guid(&self) -> &'static efi::Guid {
        &ADVANCED_LOGGER_PROTOCOL_GUID
    }
}

impl Deref for AdvancedLoggerProtocol {
    type Target = efi::Guid;

    fn deref(&self) -> &Self::Target {
        self.protocol_guid()
    }
}

// Private un-synchronized AdvancedLogger wrapper. Provides implementation of fmt::Write for AdvancedLogger.
#[derive(Debug)]
struct AdvancedLogger {
    protocol: AtomicPtr<AdvancedLoggerProtocolInterface>,
}

impl AdvancedLogger {
    // creates a new AdvancedLogger
    const fn new() -> Self {
        AdvancedLogger { protocol: AtomicPtr::new(ptr::null_mut()) }
    }

    // initialize the AdvancedLogger by acquiring a pointer to the AdvancedLogger protocol.
    fn init(&self, boot_services_impl: &impl boot_services::BootServices) {
        let protocol_ptr = match unsafe { boot_services_impl.locate_protocol(&ADVANCED_LOGGER_PROTOCOL, None) } {
            Ok(interface) => interface as *mut AdvancedLoggerProtocolInterface,
            Err(_status) => ptr::null_mut(),
        };

        self.protocol.store(protocol_ptr, Ordering::SeqCst)
    }

    // log the debug output in `args` at the given log level.
    fn log(&self, level: usize, args: fmt::Arguments) {
        let logger_ptr = self.protocol.load(Ordering::SeqCst);
        if let Some(protocol) = unsafe { logger_ptr.as_mut() } {
            let mut log_transaction = LogTransactor { protocol, level };
            log_transaction.write_fmt(args).expect("Printing to log failed.");
        }
    }
}

struct LogTransactor<'a> {
    protocol: &'a mut AdvancedLoggerProtocolInterface,
    level: usize,
}

impl<'a> fmt::Write for LogTransactor<'a> {
    fn write_str(&mut self, s: &str) -> fmt::Result {
        (self.protocol.write_log)(
            self.protocol as *const AdvancedLoggerProtocolInterface,
            self.level,
            s.as_ptr(),
            s.as_bytes().len(),
        );
        Ok(())
    }
}

/// Initializes the logging subsystem. The `debug` and `debugln` macros may be called before calling this function, but
/// output is discarded if the logger has not yet been initialized via this routine.
pub fn init_debug(efi_boot_services: *mut efi::BootServices) {
    let standard_boot_services = unsafe { boot_services::StandardBootServices::new(&*efi_boot_services) };
    LOGGER.init(&standard_boot_services);
}

#[doc(hidden)]
pub fn _log(level: usize, args: fmt::Arguments) {
    LOGGER.log(level, args)
}

#[cfg(not(feature = "std"))]
mod no_std_debug {
    /// Prints to the AdvancedLogger log at the specified level.
    ///
    /// This macro uses the same syntax as rust std [`std::println!`] macro, with the addition of a level argument that
    /// indicates what debug level the output is to be written at.
    ///
    /// See [`std::fmt`] for details on format strings.
    ///
    /// ```no_run
    /// use rust_advanced_logger_dxe::{init_debug, debug, DEBUG_INFO};
    /// use r_efi::efi::Status;
    /// pub extern "efiapi" fn efi_main(
    ///    _image_handle: *const core::ffi::c_void,
    ///    _system_table: *const r_efi::system::SystemTable,
    ///  ) -> u64 {
    ///
    ///    //Initialize debug logging - no output without this.
    ///    init_debug(unsafe { (*_system_table).boot_services});
    ///
    ///    debug!(DEBUG_INFO, "Hello, World. This is {:} in {:}. ", "rust", "UEFI");
    ///    debug!(DEBUG_INFO, "Better add our own newline.\n");
    ///
    ///    Status::SUCCESS.as_usize() as u64
    /// }
    /// ```
    #[macro_export]
    macro_rules! debug {
      ($level:expr, $($arg:tt)*) => {
          $crate::_log($level, format_args!($($arg)*))
      }
  }
}

#[cfg(feature = "std")]
mod std_debug {
    /// Prints to the console log.
    ///
    /// This macro uses the same syntax as rust std [`std::println!`] macro, with the addition of a level argument that
    /// indicates what debug level the output is to be written at.
    ///
    /// See [`std::fmt`] for details on format strings.
    ///
    /// ```no_run
    /// use rust_advanced_logger_dxe::{init_debug, debug, DEBUG_INFO};
    /// use r_efi::efi::Status;
    /// pub extern "efiapi" fn efi_main(
    ///    _image_handle: *const core::ffi::c_void,
    ///    _system_table: *const r_efi::system::SystemTable,
    ///  ) -> u64 {
    ///
    ///    //Initialize debug logging - no output without this.
    ///    init_debug(unsafe { (*_system_table).boot_services});
    ///
    ///    debug!(DEBUG_INFO, "Hello, World. This is {:} in {:}. ", "rust", "UEFI");
    ///    debug!(DEBUG_INFO, "Better add our own newline.\n");
    ///
    ///    Status::SUCCESS.as_usize() as u64
    /// }
    /// ```
    #[macro_export]
    macro_rules! debug {
      ($level:expr, $($arg:tt)*) => {
          let _ = $level;
          std::print!($($arg)*)
      }
  }
}

/// Prints to the log with a newline.
///
/// Equivalent to the [`debug!`] macro except that a newline is appended to the format string.
///
/// ```no_run
/// use rust_advanced_logger_dxe::{init_debug, debugln, DEBUG_INFO};
/// use r_efi::efi::Status;
/// pub extern "efiapi" fn efi_main(
///    _image_handle: *const core::ffi::c_void,
///    _system_table: *const r_efi::system::SystemTable,
///  ) -> u64 {
///
///    //Initialize debug logging - no output without this.
///    init_debug(unsafe { (*_system_table).boot_services});
///
///    debugln!(DEBUG_INFO, "Hello, World. This is {:} in {:}.", "rust", "UEFI");
///
///    Status::SUCCESS.as_usize() as u64
/// }
/// ```
#[macro_export]
macro_rules! debugln {
    ($level:expr) => ($crate::debug!($level, "\n"));
    ($level:expr, $fmt:expr) => ($crate::debug!($level, concat!($fmt, "\n")));
    ($level:expr, $fmt:expr, $($arg:tt)*) => ($crate::debug!($level, concat!($fmt, "\n"), $($arg)*));
}

#[cfg(test)]
mod tests {
    extern crate std;
    use crate::{
        debug, AdvancedLogger, AdvancedLoggerProtocol, AdvancedLoggerProtocolInterface, DEBUG_ERROR, DEBUG_INFO,
        DEBUG_INIT, DEBUG_VERBOSE, DEBUG_WARN, LOGGER,
    };
    use boot_services;
    use core::{slice::from_raw_parts, sync::atomic::Ordering};
    use std::{println, str};

    static ADVANCED_LOGGER_INSTANCE: AdvancedLoggerProtocolInterface =
        AdvancedLoggerProtocolInterface { signature: 0, version: 0, write_log: mock_advanced_logger_write };

    extern "efiapi" fn mock_advanced_logger_write(
        this: *const AdvancedLoggerProtocolInterface,
        error_level: usize,
        buffer: *const u8,
        buffer_size: usize,
    ) {
        assert_eq!(this, &ADVANCED_LOGGER_INSTANCE as *const AdvancedLoggerProtocolInterface);
        //to avoid dealing with complicated mock state, we assume that tests will produce the same output string:
        //"This is a <x> test.\n", where <x> depends on the debug level (e.g. "DEBUG_INFO"). In addition,
        //the string might be built with multiple calls to write, so we just check that it is a substring
        //of what we expect.
        let buf: &[u8] = unsafe { from_raw_parts(buffer, buffer_size) };
        let str = str::from_utf8(buf).unwrap();
        println!("buffer {buffer:?}:{buffer_size:?}, str: {str:?}");
        match error_level {
            DEBUG_INIT => assert!("This is a DEBUG_INIT test.\n".contains(str)),
            DEBUG_WARN => assert!("This is a DEBUG_WARN test.\n".contains(str)),
            DEBUG_INFO => assert!("This is a DEBUG_INFO test.\n".contains(str)),
            DEBUG_VERBOSE => assert!("This is a DEBUG_VERBOSE test.\n".contains(str)),
            DEBUG_ERROR => assert!("This is a DEBUG_ERROR test.\n".contains(str)),
            _ => panic!("Unrecognized error string."),
        }
    }

    #[test]
    fn init_should_initialize_logger() {
        let mut mock_boot_services = boot_services::MockBootServices::new();
        mock_boot_services.expect_locate_protocol().returning(|_: &AdvancedLoggerProtocol, registration| unsafe {
            assert_eq!(registration, None);
            Ok((&ADVANCED_LOGGER_INSTANCE as *const AdvancedLoggerProtocolInterface
                as *mut AdvancedLoggerProtocolInterface)
                .as_mut()
                .unwrap())
        });
        static TEST_LOGGER: AdvancedLogger = AdvancedLogger::new();
        TEST_LOGGER.init(&mock_boot_services);
        assert_eq!(
            TEST_LOGGER.protocol.load(Ordering::SeqCst) as *const AdvancedLoggerProtocolInterface,
            &ADVANCED_LOGGER_INSTANCE as *const AdvancedLoggerProtocolInterface
        );
    }

    #[test]
    fn debug_macro_should_log_things() {
        let mut mock_boot_services = boot_services::MockBootServices::new();
        mock_boot_services.expect_locate_protocol().returning(|_: &AdvancedLoggerProtocol, registration| unsafe {
            assert_eq!(registration, None);
            Ok((&ADVANCED_LOGGER_INSTANCE as *const AdvancedLoggerProtocolInterface
                as *mut AdvancedLoggerProtocolInterface)
                .as_mut()
                .unwrap())
        });
        LOGGER.init(&mock_boot_services);

        assert_eq!(
            LOGGER.protocol.load(Ordering::SeqCst) as *const AdvancedLoggerProtocolInterface,
            &ADVANCED_LOGGER_INSTANCE as *const AdvancedLoggerProtocolInterface
        );

        debugln!(DEBUG_INIT, "This is a DEBUG_INIT test.");
        debugln!(DEBUG_WARN, "This is a {:} test.", "DEBUG_WARN");
        debug!(DEBUG_INFO, "This {:} a {:} test.\n", "is", "DEBUG_INFO");
        debug!(DEBUG_VERBOSE, "This {:} {:} {:} test.\n", "is", "a", "DEBUG_VERBOSE");
        debug!(DEBUG_ERROR, "{:}", "This is a DEBUG_ERROR test.\n");
    }
}
