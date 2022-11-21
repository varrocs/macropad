use super::keypress::KeyPress;
use regex::Regex;

#[rustfmt::skip]
const ASCII_TO_USB_SCANCODE: [u16; 128] = [ /*
            0       1       2       3       4       5       6       7       8       9       A       B       C       D       E       F
 0 */  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,/* 0
 1 */  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,/* 1
 2 */  0x002c, 0x021E, 0x0234, 0x0220, 0x0221, 0x0222, 0x0224, 0x0034, 0x0226, 0x0227, 0x0225, 0x022e, 0x0036, 0x002d, 0x0037, 0x0038,/* 2
 3 */  0x0027, 0x001e, 0x001f, 0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0233, 0x0033, 0x0236, 0x002e, 0x0237, 0x0238,/* 3
 4 */  0x021f, 0x0204, 0x0205, 0x0206, 0x0207, 0x0208, 0x0209, 0x020A, 0x020B, 0x020C, 0x020D, 0x020E, 0x020F, 0x0210, 0x0211, 0x0212,/* 4
 5 */  0x0213, 0x0214, 0x0215, 0x0216, 0x0217, 0x0218, 0x0219, 0x021A, 0x021B, 0x021C, 0x021D, 0x0231, 0x002F, 0x0030, 0x0223, 0x022d,/* 5
 6 */  0x0035, 0x0004, 0x0005, 0x0006, 0x0007, 0x0008, 0x0009, 0x000A, 0x000B, 0x000C, 0x000D, 0x000E, 0x000F, 0x0010, 0x0011, 0x0012,/* 6
 7 */  0x0013, 0x0014, 0x0015, 0x0016, 0x0017, 0x0018, 0x0019, 0x001A, 0x001B, 0x001C, 0x001D, 0x022f, 0x0031, 0x0230, 0x0235, 0x004c,/* 7
*/
];

/*
    #define KEY_MOD_LCTRL  0x01
    #define KEY_MOD_LSHIFT 0x02
    #define KEY_MOD_LALT   0x04
    #define KEY_MOD_LMETA  0x08
    #define KEY_MOD_RCTRL  0x10
    #define KEY_MOD_RSHIFT 0x20
    #define KEY_MOD_RALT   0x40
    #define KEY_MOD_RMETA  0x80
*/

// https://gist.github.com/MightyPork/6da26e382a7ad91b5496ee55fdc73db2

fn modifier_code(code: &str) -> Option<u8> {
    match code {
        "ctrl" => Some(0x01),
        "shift" => Some(0x02),
        "alt" => Some(0x04),
        "win" => Some(0x08),
        "meta" => Some(0x08),
        _ => None,
    }
}

fn char_to_scan_code(c: char) -> Option<u16> {
    if c.is_ascii() {
        let scan_code = ASCII_TO_USB_SCANCODE[c as usize];
        Some(scan_code)
    } else {
        None
    }
}

// ------------------------------------------

fn tokenize_config_entry(input: &str) -> Vec<&str> {
    let re: Regex = Regex::new("(<([a-zA-Z0-9-z]+)>)").unwrap();
    let v: Vec<&str> = re
        .captures_iter(input)
        .map(|mat| mat.get(2).unwrap().as_str())
        .collect();
    v
}
// ctrl-shift-a
fn parse_config_token(input: &str) -> Option<KeyPress> {
    let input = input.to_lowercase();
    let subtokens: Vec<&str> = input.split("-").collect();
    let full_keypress = subtokens.iter().fold(KeyPress::default(), |acc, subtoken| {
        if let Some(mod_code) = modifier_code(subtoken) {
            //let mut acc = acc.clone();
            let mut acc = acc;
            acc.mod_byte |= mod_code;
            acc
        } else if subtoken.len() == 1 {
            let t = char_to_scan_code(subtoken.chars().nth(0).unwrap());
            if let Some(stuff) = t {
                let x: KeyPress = KeyPress::from(stuff);
                let mut acc = acc.clone();
                acc.scan_code = x.scan_code;
                acc
            } else {
                acc
            }
        } else {
            acc
        }
    });
    Some(full_keypress)
}

// -----------------------------------------------

//
// <k><a><c><s><a>
// <ctrl-shift-a><ctrl-shift-k>
pub fn parse_keypress_config(input: &str) -> Vec<KeyPress> {
    let tokens = tokenize_config_entry(input);
    tokens
        .into_iter()
        .filter_map(parse_config_token)
        .map(|kp| kp.into())
        .collect()
}

pub fn parse_ascii_config(input: &str) -> Vec<KeyPress> {
    input
        .chars()
        .filter_map(char_to_scan_code)
        .map(KeyPress::from)
        .collect()
}

#[cfg(test)]
mod tests {
    use super::*;

    fn to_code_vec(keypresses: &Vec<KeyPress>) -> Vec<u16> {
        keypresses.iter().cloned().map(KeyPress::into).collect()
    }

    fn parse_ascii_config_t(s: &str) -> Vec<u16> {
        let v = parse_ascii_config(s);
        to_code_vec(&v)
    }

    fn parse_keypress_config_t(s: &str) -> Vec<u16> {
        let v = parse_keypress_config(s);
        to_code_vec(&v)
    }

    #[test]
    fn char_to_scancode_a() {
        assert_eq!(Some(0x04), char_to_scan_code('a'));
    }

    #[test]
    fn keypress_into_u16_a() {
        assert_eq!(
            0x0004_u16,
            KeyPress {
                mod_byte: 0_u8,
                scan_code: 0x04_u8
            }
            .into()
        );
    }

    #[test]
    fn one_key_press_a() {
        assert_eq!(vec![0x04_u16], parse_ascii_config_t("a"));
    }

    #[test]
    fn multiple_key_press_asd() {
        assert_eq!(
            vec![0x04_u16, 0x16_u16, 0x07u16],
            parse_ascii_config_t("asd")
        );
        assert_eq!(
            vec![0x04_u16, 0x16_u16, 0x07u16],
            parse_keypress_config_t("<a><s><d>")
        );
    }

    #[test]
    fn parse_keypress_config_a() {
        assert_eq!(vec![0x0004], parse_keypress_config_t("<a>"));
    }

    #[test]
    fn parse_keypress_config_ctrl_a() {
        assert_eq!(vec![0x0104], parse_keypress_config_t("<ctrl-a>"));
        assert_eq!(vec![0x0104], parse_keypress_config_t("<ctrl-A>"));
        assert_eq!(vec![0x0104], parse_keypress_config_t("<Ctrl-A>"));

        assert_eq!(vec![0x0304], parse_keypress_config_t("<shift-ctrl-a>"));
        assert_eq!(vec![0x0304], parse_keypress_config_t("<ctrl-shift-a>"));
        assert_eq!(
            vec![0x0104, 0x0205],
            parse_keypress_config_t("<ctrl-a><shift-b>")
        );
    }
}
