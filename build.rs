use glob::glob;
use std::path::PathBuf;
extern crate bindgen;

use std::env;
use std::error::Error;

fn main() -> Result<(), Box<dyn Error>> {
  let manifest_dir = env::var("CARGO_MANIFEST_DIR")?;
  let release_or_debug = if cfg!(debug_assertions) { "debug" } else { "release" };
  let arduino_core_path = "/Users/linusoleander/.platformio/packages/framework-arduinoespressif32/cores/esp32";
  let xtensa_include_path = "/Users/linusoleander/.rustup/toolchains/esp/xtensa-esp32-elf/esp-12.2.0_20230208/xtensa-esp32-elf/xtensa-esp32-elf/include";

  let pio_libdeps_path = PathBuf::from(manifest_dir).join(".pio").join("libdeps").join(release_or_debug);

  // Iterate over each dependency's include directory to find header files
  for entry in glob(pio_libdeps_path.join("*/include/*.h").to_str().unwrap())?
    .chain(glob(pio_libdeps_path.join("*/src/*.h").to_str().unwrap())?)
  {
    let path = entry?;
    println!("cargo:rerun-if-changed={}", path.to_str().unwrap());

    // get all arguments from CARGO_PIO_BUILD_BINDGEN_EXTRA_CLANG_ARGS
    let mut clang_args = vec![];
    if let Ok(extra_args) = env::var("CARGO_PIO_BUILD_BINDGEN_EXTRA_CLANG_ARGS") {
      for arg in extra_args.split_whitespace() {
        clang_args.push(arg.to_string());
      }
    }

    let bindings = bindgen::Builder::default()
      .header(path.to_str().unwrap())
      .clang_arg(format!("-I{}", arduino_core_path))
      .clang_arg(format!("-I{}", xtensa_include_path))
      .clang_args(clang_args)
      .generate_comments(false)
      .derive_default(true)
      .generate()
      .expect("Unable to generate bindings");

    let out_path = PathBuf::from(env::var("OUT_DIR")?);
    bindings.write_to_file(out_path.join("bindings.rs"))?;
  }
  Ok(())
}
