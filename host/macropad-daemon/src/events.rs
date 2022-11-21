use std::path::PathBuf;

pub enum Messages {
    ConfigChanged,
    MacropadDetected(PathBuf),
    MacropadUnplugged,
}
