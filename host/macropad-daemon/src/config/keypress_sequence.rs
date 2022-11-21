use serde::Deserialize;
use std::fmt;
// ------------------------------------------
use serde::de::{self, Deserializer, Visitor};

use super::keypress::KeyPress;
use super::keypress_config_parser::{parse_ascii_config, parse_keypress_config};

#[derive(Clone, Default, Debug, PartialEq)]
pub struct AsciiKeyPressSequence(Vec<KeyPress>);

#[derive(Clone, Default, Debug, PartialEq)]
pub struct SeqKeyPressSequence(Vec<KeyPress>);

impl Into<Vec<u16>> for AsciiKeyPressSequence {
    fn into(self) -> Vec<u16> {
        self.0.into_iter().map(KeyPress::into).collect()
    }
}

impl Into<Vec<u16>> for SeqKeyPressSequence {
    fn into(self) -> Vec<u16> {
        self.0.into_iter().map(KeyPress::into).collect()
    }
}

struct AsciiKeyPressVisitor {}
struct SeqKeyPressVisitor {}

impl<'de> Deserialize<'de> for AsciiKeyPressSequence {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        deserializer.deserialize_str(AsciiKeyPressVisitor {})
    }
}

impl<'de> Deserialize<'de> for SeqKeyPressSequence {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        deserializer.deserialize_str(SeqKeyPressVisitor {})
    }
}

impl<'de> Visitor<'de> for AsciiKeyPressVisitor {
    type Value = AsciiKeyPressSequence;

    fn expecting(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
        formatter.write_str("expected an ascii string")
    }

    fn visit_str<E>(self, value: &str) -> Result<Self::Value, E>
    where
        E: de::Error,
    {
        Ok(AsciiKeyPressSequence(parse_ascii_config(value)))
    }
}

impl<'de> Visitor<'de> for SeqKeyPressVisitor {
    type Value = SeqKeyPressSequence;

    fn expecting(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
        formatter.write_str("expected an ascii string")
    }

    fn visit_str<E>(self, value: &str) -> Result<Self::Value, E>
    where
        E: de::Error,
    {
        Ok(SeqKeyPressSequence(parse_keypress_config(value)))
    }
}
