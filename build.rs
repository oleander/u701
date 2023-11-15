fn main() -> Result<(), Box<dyn std::error::Error>> {
  embuild::build::CfgArgs::output_propagated("ESP_IDF")?;
  embuild::build::LinkArgs::output_propagated("ESP_IDF")?;

  Ok(())
}
