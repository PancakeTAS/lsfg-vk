use gtk::subclass::prelude::*;
use adw::subclass::prelude::*;
use gtk::{glib, CompositeTemplate};

#[derive(CompositeTemplate, Default)]
#[template(resource = "/gay/pancake/lsfg-vk/popup/process.ui")]
pub struct ProcessPicker {
    #[template_child]
    pub processes: TemplateChild<gtk::ListBox>,
    #[template_child]
    pub close: TemplateChild<gtk::Button>,
}

#[glib::object_subclass]
impl ObjectSubclass for ProcessPicker {
    const NAME: &'static str = "LSProcessPicker";
    type Type = super::ProcessPicker;
    type ParentType = adw::ApplicationWindow;

    fn class_init(klass: &mut Self::Class) {
        klass.bind_template();
    }

    fn instance_init(obj: &glib::subclass::InitializingObject<Self>) {
        obj.init_template();
    }
}

impl ObjectImpl for ProcessPicker {
    fn constructed(&self) {
        self.parent_constructed();
    }
}

impl WidgetImpl for ProcessPicker {}
impl WindowImpl for ProcessPicker {}
impl ApplicationWindowImpl for ProcessPicker {}
impl AdwWindowImpl for ProcessPicker {}
impl AdwApplicationWindowImpl for ProcessPicker {}
