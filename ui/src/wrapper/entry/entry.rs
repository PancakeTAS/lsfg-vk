use std::cell::RefCell;

use gtk::glib;
use gtk::subclass::prelude::*;
use gtk::prelude::*;

#[derive(gtk::CompositeTemplate, glib::Properties, Default)]
#[properties(wrapper_type = super::Entry)]
#[template(resource = "/gay/pancake/lsfg-vk/entry.ui")]
pub struct Entry {
    #[property(get, set)]
    exe: RefCell<String>,

    #[template_child]
    pub delete: TemplateChild<gtk::Button>,
}

#[glib::object_subclass]
impl ObjectSubclass for Entry {
    const NAME: &'static str = "LSEntry";
    type Type = super::Entry;
    type ParentType = gtk::ListBoxRow;

    fn class_init(klass: &mut Self::Class) {
        klass.bind_template();
    }

    fn instance_init(obj: &glib::subclass::InitializingObject<Self>) {
        obj.init_template();
    }
}

#[glib::derived_properties]
impl ObjectImpl for Entry {
    fn constructed(&self) {
        self.parent_constructed();
    }
}

impl WidgetImpl for Entry {}
impl ListBoxRowImpl for Entry {}
