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

#[cfg(doc)]
extern crate std; //allow rustdoc links to reference std (e.g. println docs below).

use core::{
  ffi::c_void,
  fmt::{self, Write},
  sync::atomic::{AtomicPtr, Ordering},
};
use r_efi::{
  efi::{Guid, Status},
  system::BootServices,
};

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
const ADVANCED_LOGGER_PROTOCOL_GUID: Guid =
  Guid::from_fields(0x434f695c, 0xef26, 0x4a12, 0x9e, 0xba, &[0xdd, 0xef, 0x00, 0x97, 0x49, 0x7c]);

type AdvancedLoggerWriteProtocol = extern "efiapi" fn(*const AdvancedLoggerProtocol, usize, *const u8, usize);

#[repr(C)]
struct AdvancedLoggerProtocol {
  signature: u32,
  version: u32,
  write_log: AdvancedLoggerWriteProtocol,
}

// Private un-synchronized AdvancedLogger wrapper. Provides implementation of fmt::Write for AdvancedLogger.
#[derive(Debug)]
struct AdvancedLogger {
  protocol: AtomicPtr<AdvancedLoggerProtocol>,
}

impl AdvancedLogger {
  // creates a new AdvancedLogger
  const fn new() -> Self {
    AdvancedLogger { protocol: AtomicPtr::new(core::ptr::null_mut()) }
  }

  // initialize the AdvancedLogger by acquiring a pointer to the AdvancedLogger protocol.
  fn init(&self, bs: *mut BootServices) {
    let boot_services = unsafe { bs.as_mut().expect("Boot Services Pointer is NULL") };
    let mut ptr: *mut c_void = core::ptr::null_mut();
    let status = (boot_services.locate_protocol)(
      &ADVANCED_LOGGER_PROTOCOL_GUID as *const Guid as *mut Guid,
      core::ptr::null_mut(),
      core::ptr::addr_of_mut!(ptr),
    );
    match status {
      Status::SUCCESS => self.protocol.store(ptr as *mut AdvancedLoggerProtocol, Ordering::SeqCst),
      _ => self.protocol.store(core::ptr::null_mut(), Ordering::SeqCst),
    }
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

unsafe impl Sync for AdvancedLogger {}
unsafe impl Send for AdvancedLogger {}

struct LogTransactor<'a> {
  protocol: &'a mut AdvancedLoggerProtocol,
  level: usize,
}

impl<'a> fmt::Write for LogTransactor<'a> {
  fn write_str(&mut self, s: &str) -> fmt::Result {
    (self.protocol.write_log)(
      self.protocol as *const AdvancedLoggerProtocol,
      self.level,
      s.as_ptr(),
      s.as_bytes().len(),
    );
    Ok(())
  }
}

/// Initializes the logging subsystem. The `debug` and `debugln` macros may be called before calling this function, but
/// output is discarded if the logger has not yet been initialized via this routine.
pub fn init_debug(bs: *mut BootServices) {
  LOGGER.init(bs);
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
  extern crate std;

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
          print!($($arg)*)
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

/// Yields a &'static str that is the name of the containing function.
#[macro_export]
macro_rules! function {
  () => {{
    fn f() {}
    fn type_name_of<T>(_: T) -> &'static str {
      core::any::type_name::<T>()
    }
    let name = type_name_of(f);
    name.strip_suffix("::f").unwrap()
  }};
}

#[cfg(test)]
mod tests {
  extern crate std;
  use crate::{
    debug, init_debug, AdvancedLogger, AdvancedLoggerProtocol, ADVANCED_LOGGER_PROTOCOL_GUID, DEBUG_ERROR, DEBUG_INFO,
    DEBUG_INIT, DEBUG_VERBOSE, DEBUG_WARN, LOGGER,
  };
  use core::{ffi::c_void, mem::MaybeUninit, slice::from_raw_parts, sync::atomic::Ordering};
  use r_efi::{
    efi::{Guid, Status},
    system::BootServices,
  };
  use std::{println, str};

  static ADVANCED_LOGGER_INSTANCE: AdvancedLoggerProtocol =
    AdvancedLoggerProtocol { signature: 0, version: 0, write_log: mock_advanced_logger_write };

  extern "efiapi" fn mock_advanced_logger_write(
    this: *const AdvancedLoggerProtocol,
    error_level: usize,
    buffer: *const u8,
    buffer_size: usize,
  ) {
    assert_eq!(this, &ADVANCED_LOGGER_INSTANCE as *const AdvancedLoggerProtocol);
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

  extern "efiapi" fn mock_locate_protocol(
    protocol: *mut Guid,
    _registration: *mut c_void,
    interface: *mut *mut c_void,
  ) -> Status {
    let protocol = unsafe { protocol.as_mut().unwrap() };
    assert_eq!(protocol, &ADVANCED_LOGGER_PROTOCOL_GUID);
    assert!(!interface.is_null());
    unsafe {
      interface.write(&ADVANCED_LOGGER_INSTANCE as *const AdvancedLoggerProtocol as *mut c_void);
    }
    Status::SUCCESS
  }

  fn mock_boot_services() -> BootServices {
    let boot_services = MaybeUninit::zeroed();
    let mut boot_services: BootServices = unsafe { boot_services.assume_init() };
    boot_services.locate_protocol = mock_locate_protocol;
    boot_services
  }

  #[test]
  fn init_should_initialize_logger() {
    let mut boot_services = mock_boot_services();
    static TEST_LOGGER: AdvancedLogger = AdvancedLogger::new();
    TEST_LOGGER.init(&mut boot_services);

    assert_eq!(
      TEST_LOGGER.protocol.load(Ordering::SeqCst) as *const AdvancedLoggerProtocol,
      &ADVANCED_LOGGER_INSTANCE as *const AdvancedLoggerProtocol
    );
  }

  #[test]
  fn debug_macro_should_log_things() {
    let mut boot_services = mock_boot_services();
    init_debug(&mut boot_services);

    assert_eq!(
      LOGGER.protocol.load(Ordering::SeqCst) as *const AdvancedLoggerProtocol,
      &ADVANCED_LOGGER_INSTANCE as *const AdvancedLoggerProtocol
    );

    debugln!(DEBUG_INIT, "This is a DEBUG_INIT test.");
    debugln!(DEBUG_WARN, "This is a {:} test.", "DEBUG_WARN");
    debug!(DEBUG_INFO, "This {:} a {:} test.\n", "is", "DEBUG_INFO");
    debug!(DEBUG_VERBOSE, "This {:} {:} {:} test.\n", "is", "a", "DEBUG_VERBOSE");
    debug!(DEBUG_ERROR, "{:}", "This is a DEBUG_ERROR test.\n");
  }
}
