use keypress_sequence::{AsciiKeyPressSequence, SeqKeyPressSequence};
use serde::Deserialize;

mod keypress;
mod keypress_config_parser;
mod keypress_sequence;

// ------------------------

#[derive(Debug, PartialEq, Deserialize)]
pub struct KeyConfig {
    pub key: u8,
    pub ascii: Option<AsciiKeyPressSequence>,
    pub keypress: Option<SeqKeyPressSequence>,
}

#[derive(Debug, PartialEq, Deserialize)]
pub struct Layout {
    pub name: String,
    pub keys: Vec<KeyConfig>,
}

#[derive(Debug, PartialEq, Deserialize)]
pub struct Config {
    pub current_layout: String,
    pub layouts: Vec<Layout>,
}

impl Config {
    pub fn get_current_layout(&self) -> Option<&Layout> {
        self.layouts.iter().find(|l| l.name == self.current_layout)
    }

    pub fn set_current_layout(&mut self, layout: &str) -> Result<(), &'static str> {
        let existing_layout = self.layouts.iter().find(|l| l.name == layout);
        existing_layout.ok_or("Unknown layout")?;

        self.current_layout = String::from(layout);
        Ok(())
    }

    pub fn list_layout_names(&self) -> Vec<String> {
        self.layouts.iter().map(|l| l.name.clone()).collect()
    }
}

pub fn load_config(config: &str) -> Result<Config, String> {
    let config: Result<Config, String> = serde_yaml::from_str(config).map_err(|e| {
        format!(
            "Failed to load config {e}, location: {}",
            e.location()
                .map(|l| format!(
                    "index: {}, line: {}, column: {}",
                    l.index(),
                    l.line(),
                    l.column()
                ))
                .unwrap_or("N/A".to_owned())
        )
    });
    config
}

#[cfg(test)]
mod tests {
    use super::*;

    const TEST_CONFIG_1: &str = "
        current_layout: \"layout_1\"
        layouts:
         - name: layout_1
           keys:
           - key: 0
             ascii: \"asd\"
           - key: 1
             keypress: \"<ctrl-a>\"
         - name: layout_2
           keys:
           - key: 0
             keypress: \"<ctrl-shift-a>\"
           - key: 2
             ascii: \"eeee\"
        ";

    #[test]
    fn load_config_yml() {
        let config: Result<Config, _> = load_config(TEST_CONFIG_1);
        //let config: Result<Config, _> = serde_yaml::from_str(TEST_CONFIG_1);
        assert_eq!(true, config.is_ok());
        let config = config.unwrap();

        assert_eq!("layout_1", config.current_layout);

        assert_eq!(2, config.layouts.len());
        assert_eq!("layout_1", config.layouts[0].name);
        assert_eq!("layout_2", config.layouts[1].name);

        assert_eq!(0, config.layouts[0].keys[0].key);
        assert_eq!(1, config.layouts[0].keys[1].key);
        assert_eq!(0, config.layouts[1].keys[0].key);
        assert_eq!(2, config.layouts[1].keys[1].key);

        assert_eq!(None, config.layouts[0].keys[0].keypress);
        assert_eq!(None, config.layouts[0].keys[1].ascii);

        let result: Vec<u16> = config.layouts[0].keys[0].ascii.clone().unwrap().into();
        assert_eq!(vec![0x04, 22, 7], result);
        let result: Vec<u16> = config.layouts[0].keys[1].keypress.clone().unwrap().into();
        assert_eq!(vec![0x0104_u16], result);
    }

    #[test]
    fn load_and_parse_config_yml() {
        let config: Result<Config, _> = load_config(TEST_CONFIG_1);
        //let config: Result<Config, _> = serde_yaml::from_str(TEST_CONFIG_1);
        assert_eq!(true, config.is_ok());
        let config = config.unwrap();

        let result: Vec<u16> = config.layouts[0].keys[0].ascii.clone().unwrap().into();
        assert_eq!(vec![0x04, 22, 7], result);
        assert_eq!(None, config.layouts[0].keys[0].keypress);
        assert_eq!(None, config.layouts[0].keys[1].ascii);
        let result: Vec<u16> = config.layouts[0].keys[1].keypress.clone().unwrap().into();
        assert_eq!(vec![0x0104_u16], result);
    }
}
