use std::cell::RefCell;

use gtk::glib;
use gtk::subclass::prelude::*;
use adw::subclass::prelude::*;
use adw::prelude::*;

#[derive(gtk::CompositeTemplate, glib::Properties, Default)]
#[properties(wrapper_type = super::PrefEntry)]
#[template(resource = "/gay/pancake/lsfg-vk/pref/entry.ui")]
pub struct PrefEntry {
    #[property(get, set)]
    opt_name: RefCell<String>,
    #[property(get, set)]
    default_text: RefCell<String>,
    #[property(get, set)]
    tooltip_text: RefCell<String>
}

#[glib::object_subclass]
impl ObjectSubclass for PrefEntry {
    const NAME: &'static str = "LSPrefEntry";
    type Type = super::PrefEntry;
    type ParentType = adw::PreferencesRow;

    fn class_init(klass: &mut Self::Class) {
        klass.bind_template();
    }

    fn instance_init(obj: &glib::subclass::InitializingObject<Self>) {
        obj.init_template();
    }
}

#[glib::derived_properties]
impl ObjectImpl for PrefEntry {
    fn constructed(&self) {
        self.parent_constructed();
    }
}

impl WidgetImpl for PrefEntry {}
impl ListBoxRowImpl for PrefEntry {}
impl PreferencesRowImpl for PrefEntry {}
