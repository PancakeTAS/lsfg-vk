use std::cell::RefCell;

use gtk::glib;
use gtk::subclass::prelude::*;
use gtk::prelude::*;

#[derive(gtk::CompositeTemplate, glib::Properties, Default)]
#[properties(wrapper_type = super::Entry)]
#[template(resource = "/gay/pancake/lsfg-vk/entry/entry.ui")]
pub struct Entry {
    #[property(get, set)]
    exe: RefCell<String>,
    #[property(get, set)]
    name: RefCell<Option<String>>,

    #[template_child]
    pub label: TemplateChild<gtk::Label>,
    #[template_child]
    pub delete: TemplateChild<gtk::Button>,
    #[template_child]
    pub edit: TemplateChild<gtk::Button>,
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
        
        // Update label when properties change
        let obj = self.obj();
        obj.connect_notify_local(Some("exe"), glib::clone!(@weak obj => move |_, _| {
            obj.imp().update_label();
        }));
        obj.connect_notify_local(Some("name"), glib::clone!(@weak obj => move |_, _| {
            obj.imp().update_label();
        }));
        
        // Set initial label
        self.update_label();
    }
}

impl Entry {
    /// Get the display name (custom name if set, otherwise exe name)
    pub fn display_name(&self) -> String {
        if let Some(name) = self.name() {
            if !name.trim().is_empty() {
                return name;
            }
        }
        self.exe()
    }

    /// Update the label to show the current display name
    pub fn update_label(&self) {
        self.label.set_text(&self.display_name());
    }
}

impl WidgetImpl for Entry {}
impl ListBoxRowImpl for Entry {}
