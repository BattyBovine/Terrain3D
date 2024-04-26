// Copyright © 2023 Cory Petkovsek, Roope Palmroos, and Contributors.

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_paths.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/project_settings.hpp>

#include "logger.h"
#include "terrain_3d_mesh_instance.h"

///////////////////////////
// Private Functions
///////////////////////////

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

void Terrain3DMeshInstance::set_scene_file(const Ref<PackedScene> p_scene_file) {
	LOG(MESG, "Setting scene file and instantiating node");
	_packed_scene = p_scene_file;
	if (_packed_scene.is_valid()) {
		_scene_node = _packed_scene->instantiate();
		LOG(DEBUG, "Loaded scene with parent node: ", _scene_node);
		TypedArray<Node> mesh_instances = _scene_node->find_children("*", "MeshInstance3D");
		_meshes.clear();
		for (int i = 0; i < mesh_instances.size(); i++) {
			MeshInstance3D *mi = Object::cast_to<MeshInstance3D>(mesh_instances[i]);
			LOG(MESG, "Found mesh: ", mi->get_name());
			Ref<Mesh> mesh = mi->get_mesh();
			for (int j = 0; j < mi->get_surface_override_material_count(); j++) {
				Ref<Material> mat = mi->get_active_material(j);
				mesh->surface_set_material(j, mat);
			}
			_meshes.push_back(mesh);
		}
	}
	emit_signal("file_changed");
}

Ref<Texture2D> Terrain3DMeshInstance::get_thumbnail() const {
	if (_packed_scene.is_null()) {
		return Ref<Texture2D>();
	}

	// Copied from godot\editor\plugins\editor_preview_plugins.cpp:EditorPackedScenePreviewPlugin::generate_from_path
	String scene_path = _packed_scene->get_path();
	String temp_path = EditorInterface::get_singleton()->get_editor_paths()->get_cache_dir();
	String cache_base = ProjectSettings::get_singleton()->globalize_path(scene_path).md5_text();
	cache_base = temp_path.path_join("resthumb-" + cache_base);
	String png_path = cache_base + ".png";
	if (FileAccess::file_exists(png_path)) {
		Ref<Image> img;
		img.instantiate();
		Error err = img->load(png_path);
		if (err == OK) {
			return ImageTexture::create_from_image(img);
		}
	}
	return Ref<Texture2D>();
}

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
	ClassDB::bind_method(D_METHOD("set_scene_file", "scene_file"), &Terrain3DMeshInstance::set_scene_file);
	ClassDB::bind_method(D_METHOD("get_scene_file"), &Terrain3DMeshInstance::get_scene_file);
	ClassDB::bind_method(D_METHOD("get_thumbnail"), &Terrain3DMeshInstance::get_thumbnail);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "name", PROPERTY_HINT_NONE), "set_name", "get_name");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "id", PROPERTY_HINT_NONE), "set_id", "get_id");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "scene_file", PROPERTY_HINT_RESOURCE_TYPE, "PackedScene"), "set_scene_file", "get_scene_file");
}
