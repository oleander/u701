use glob::glob;
use std::path::PathBuf;
use std::env;

// Necessary because of this issue: https://github.com/rust-lang/cargo/issues/9641
fn main() -> Result<(), Box<dyn std::error::Error>> {
  embuild::build::CfgArgs::output_propagated("ESP_IDF")?;
  embuild::build::LinkArgs::output_propagated("ESP_IDF")?;

  let env = "release";

  let pattern = PathBuf::from(env::var("CARGO_MANIFEST_DIR").unwrap()).join(".pio").join("libdeps").join(env).join("**").join("include").join("*.h");

  for entry in glob(pattern.to_str().unwrap()).expect("Failed to read glob pattern") {
    match entry {
      Ok(path) => {
        println!("Found header: {:?}", path.display());
        // You can now call bindgen for each header file or collect them
      },
      Err(e) => println!("Error: {:?}", e)
    }
  }

  Ok(())
}
