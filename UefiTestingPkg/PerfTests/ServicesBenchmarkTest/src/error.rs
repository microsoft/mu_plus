pub enum BenchError {
    InvalidData(&'static str),
    BenchFnFailure(&'static str),
}
