use gtk::glib;
use gtk;
use adw;

pub mod main;
pub mod sidebar;

glib::wrapper! {
    pub struct PaneMain(ObjectSubclass<main::PaneMain>)
        @extends
            adw::NavigationPage, gtk::Widget,
        @implements
            gtk::Accessible, gtk::Actionable, gtk::Buildable, gtk::ConstraintTarget;
}

glib::wrapper! {
    pub struct PaneSidebar(ObjectSubclass<sidebar::PaneSidebar>)
        @extends
            adw::NavigationPage, gtk::Widget,
        @implements
            gtk::Accessible, gtk::Actionable, gtk::Buildable, gtk::ConstraintTarget;
}


impl PaneMain {
    pub fn new() -> Self {
        glib::Object::new()
    }
}

impl PaneSidebar {
    pub fn new() -> Self {
        glib::Object::new()
    }
}
