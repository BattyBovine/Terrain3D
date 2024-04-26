// Copyright © 2023 Cory Petkovsek, Roope Palmroos, and Contributors.

//#include <godot_cpp/classes/image.hpp>

#include "terrain_3d_mesh_instance.h"
#include "logger.h"

///////////////////////////
// Private Functions
///////////////////////////

//bool Terrain3DMeshInstance::_is_texture_valid(const Ref<Texture2D> &p_texture) const {
//	if (p_texture.is_null()) {
//		LOG(DEBUG, "Provided texture is null.");
//		return true;
//	}
//
//	Ref<Image> img = p_texture->get_image();
//	Image::Format format = Image::FORMAT_MAX;
//	if (img.is_valid()) {
//		format = img->get_format();
//	}
//	if (format < 0 || format >= Image::FORMAT_MAX) {
//		LOG(ERROR, "Invalid texture format. See documentation for format specification.");
//		return false;
//	}
//
//	return true;
//}

///////////////////////////
// Public Functions
///////////////////////////

Terrain3DMeshInstance::Terrain3DMeshInstance() {
	clear();
}

Terrain3DMeshInstance::~Terrain3DMeshInstance() {
}

void Terrain3DMeshInstance::clear() {
	_name = "New Mesh";
	_id = 0;
	_packed_scene.unref();
}

void Terrain3DMeshInstance::set_name(String p_name) {
	_name = p_name;
	emit_signal("setting_changed");
}

void Terrain3DMeshInstance::set_id(int p_new_id) {
	int old_id = _id;
	_id = p_new_id;
	emit_signal("id_changed", Terrain3DAssets::TYPE_MESH, old_id, p_new_id);
}

void Terrain3DMeshInstance::set_scene(const Ref<PackedScene> p_scene) {
	_packed_scene = p_scene;
	emit_signal("file_changed");
}

//void Terrain3DMeshInstance::set_albedo_color(Color p_color) {
//	_data._albedo_color = p_color;
//	emit_signal("setting_changed");
//}
//
//void Terrain3DMeshInstance::set_albedo_texture(const Ref<Texture2D> &p_texture) {
//	if (_is_texture_valid(p_texture)) {
//		_data._albedo_texture = p_texture;
//		emit_signal("file_changed");
//	}
//}
//
//void Terrain3DMeshInstance::set_normal_texture(const Ref<Texture2D> &p_texture) {
//	if (_is_texture_valid(p_texture)) {
//		_data._normal_texture = p_texture;
//		emit_signal("file_changed");
//	}
//}
//
//void Terrain3DMeshInstance::set_uv_scale(real_t p_scale) {
//	_data._uv_scale = p_scale;
//	emit_signal("setting_changed");
//}
//
//void Terrain3DMeshInstance::set_uv_rotation(real_t p_rotation) {
//	_data._uv_rotation = CLAMP(p_rotation, 0.0f, 1.0f);
//	emit_signal("setting_changed");
//}

///////////////////////////
// Protected Functions
///////////////////////////

void Terrain3DMeshInstance::_bind_methods() {
	ADD_SIGNAL(MethodInfo("id_changed"));
	ADD_SIGNAL(MethodInfo("file_changed"));
	ADD_SIGNAL(MethodInfo("setting_changed"));

	ClassDB::bind_method(D_METHOD("clear"), &Terrain3DMeshInstance::clear);
	ClassDB::bind_method(D_METHOD("set_name", "name"), &Terrain3DMeshInstance::set_name);
	ClassDB::bind_method(D_METHOD("get_name"), &Terrain3DMeshInstance::get_name);
	ClassDB::bind_method(D_METHOD("set_id", "id"), &Terrain3DMeshInstance::set_id);
	ClassDB::bind_method(D_METHOD("get_id"), &Terrain3DMeshInstance::get_id);
	ClassDB::bind_method(D_METHOD("set_scene", "scene"), &Terrain3DMeshInstance::set_scene);
	ClassDB::bind_method(D_METHOD("get_scene"), &Terrain3DMeshInstance::get_scene);
	//ClassDB::bind_method(D_METHOD("set_albedo_color", "color"), &Terrain3DMeshInstance::set_albedo_color);
	//ClassDB::bind_method(D_METHOD("get_albedo_color"), &Terrain3DMeshInstance::get_albedo_color);
	//ClassDB::bind_method(D_METHOD("set_albedo_texture", "texture"), &Terrain3DMeshInstance::set_albedo_texture);
	//ClassDB::bind_method(D_METHOD("get_albedo_texture"), &Terrain3DMeshInstance::get_albedo_texture);
	//ClassDB::bind_method(D_METHOD("set_normal_texture", "texture"), &Terrain3DMeshInstance::set_normal_texture);
	//ClassDB::bind_method(D_METHOD("get_normal_texture"), &Terrain3DMeshInstance::get_normal_texture);
	//ClassDB::bind_method(D_METHOD("set_uv_scale", "scale"), &Terrain3DMeshInstance::set_uv_scale);
	//ClassDB::bind_method(D_METHOD("get_uv_scale"), &Terrain3DMeshInstance::get_uv_scale);
	//ClassDB::bind_method(D_METHOD("set_uv_rotation", "scale"), &Terrain3DMeshInstance::set_uv_rotation);
	//ClassDB::bind_method(D_METHOD("get_uv_rotation"), &Terrain3DMeshInstance::get_uv_rotation);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "name", PROPERTY_HINT_NONE), "set_name", "get_name");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "id", PROPERTY_HINT_NONE), "set_id", "get_id");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "scene", PROPERTY_HINT_NONE), "set_scene", "get_scene");
	//ADD_PROPERTY(PropertyInfo(Variant::COLOR, "albedo_color", PROPERTY_HINT_COLOR_NO_ALPHA), "set_albedo_color", "get_albedo_color");
	//ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "albedo_texture", PROPERTY_HINT_RESOURCE_TYPE, "ImageTexture,CompressedTexture2D"), "set_albedo_texture", "get_albedo_texture");
	//ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "normal_texture", PROPERTY_HINT_RESOURCE_TYPE, "ImageTexture,CompressedTexture2D"), "set_normal_texture", "get_normal_texture");
	//ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "uv_scale", PROPERTY_HINT_RANGE, "0.001, 2.0"), "set_uv_scale", "get_uv_scale");
	//ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "uv_rotation", PROPERTY_HINT_RANGE, "0.0, 1.0"), "set_uv_rotation", "get_uv_rotation");
}
