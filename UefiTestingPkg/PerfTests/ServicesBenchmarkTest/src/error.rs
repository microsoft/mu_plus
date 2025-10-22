use core::fmt;

use r_efi::efi;

#[derive(Debug)]
pub enum BenchError {
    BenchSetupFailure(&'static str, efi::Status),
    BenchFailure(&'static str, efi::Status),
    BenchCleanupFailure(&'static str, efi::Status),
    WriteFailure(&'static str, core::fmt::Error),
}

impl fmt::Display for BenchError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            BenchError::BenchSetupFailure(msg, status)
            | BenchError::BenchFailure(msg, status)
            | BenchError::BenchCleanupFailure(msg, status) => {
                write!(f, "{} with error {:?}", msg, status)
            }
            BenchError::WriteFailure(msg, err) => {
                write!(f, "{} with formatting error {:?}", msg, err)
            }
        }
    }
}
