fn main() {
    glib_build_tools::compile_resources(
        &["rsc"],
        "rsc/resources.gresource.xml",
        "ui.gresource",
    );
}
