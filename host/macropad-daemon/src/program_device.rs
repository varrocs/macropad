use super::config::{KeyConfig, Layout};
use std::fs::{File, OpenOptions};
use std::io::{Result, Write};
use std::path::Path;
use std::thread;
use std::time;

fn set_key(f: &mut File, key: &KeyConfig) -> Result<()> {
    let sequence: Option<Vec<u16>> = if key.ascii.is_some() {
        Some(key.ascii.clone().unwrap().into())
    } else if key.keypress.is_some() {
        Some(key.keypress.clone().unwrap().into())
    } else {
        None
    };
    // No config for this key
    if sequence.is_none() {
        return Ok(());
    }

    // Set up the stuff to send
    let buffer: Vec<String> = sequence
        .unwrap()
        .iter()
        .map(|e: &u16| format!("{}", e))
        .collect();
    let buffer = buffer.join(" ");
    let buffer = format!("set {} {}\r\n", key.key, buffer);

    thread::sleep(time::Duration::from_millis(100));
    println!("Command: '{}'", buffer);
    f.write_all(buffer.as_bytes())?;
    thread::sleep(time::Duration::from_millis(100));
    /*
    let mut response = String::new();
    f.read_to_string(&mut response)?;
    println!("Response: '{}'", response);
    */
    Ok(())
}

pub fn setup_device(layout: &Layout, device: &Path) -> Result<()> {
    //let mut f = File::open(device)?;
    let mut f = OpenOptions::new().read(true).write(true).open(device)?;

    for key in layout.keys.iter() {
        let result = set_key(&mut f, &key);
        if result.is_err() {
            println!(
                "Failed to set key {}, err: {}",
                key.key,
                result.unwrap_err()
            );
        }
    }

    Ok(())
}
