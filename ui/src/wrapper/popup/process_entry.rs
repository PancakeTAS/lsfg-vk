use std::cell::RefCell;

use gtk::glib;
use gtk::subclass::prelude::*;
use gtk::prelude::*;

#[derive(gtk::CompositeTemplate, glib::Properties, Default)]
#[properties(wrapper_type = super::ProcessEntry)]
#[template(resource = "/gay/pancake/lsfg-vk/popup/process_entry.ui")]
pub struct ProcessEntry {
    #[property(get, set)]
    exe: RefCell<String>,
}

#[glib::object_subclass]
impl ObjectSubclass for ProcessEntry {
    const NAME: &'static str = "LSProcessEntry";
    type Type = super::ProcessEntry;
    type ParentType = gtk::ListBoxRow;

    fn class_init(klass: &mut Self::Class) {
        klass.bind_template();
    }

    fn instance_init(obj: &glib::subclass::InitializingObject<Self>) {
        obj.init_template();
    }
}

#[glib::derived_properties]
impl ObjectImpl for ProcessEntry {
    fn constructed(&self) {
        self.parent_constructed();
    }
}

impl WidgetImpl for ProcessEntry {}
impl ListBoxRowImpl for ProcessEntry {}
