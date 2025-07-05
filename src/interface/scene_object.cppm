// SPDX-FileCopyrightText: (c) 2024 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "definitions.hpp"
#include <sharedutils/util_weak_handle.hpp>
#include <sharedutils/util.h>
#include <memory>

export module pragma.scenekit:scene_object;

export namespace pragma::scenekit {
	class Scene;
	class DLLRTUTIL BaseObject {
	  public:
		BaseObject();
		virtual ~BaseObject();
		void Finalize(Scene &scene, bool force = false);

		void SetHash(const util::MurmurHash3 &hash) { m_hash = hash; }
		void SetHash(util::MurmurHash3 &&hash) { m_hash = std::move(hash); }
		const util::MurmurHash3 &GetHash() const { return m_hash; }
		const std::string &GetName() const { return m_name; }
		void SetName(const std::string &name) { m_name = name; }
		uint32_t GetId() const { return m_id; }
		void SetId(uint32_t id) { m_id = id; }
	  protected:
		friend pragma::scenekit::Scene;
		virtual void DoFinalize(Scene &scene);
	  private:
		bool m_bFinalized = false;
		std::string m_name;
		util::MurmurHash3 m_hash {};
		uint32_t m_id = std::numeric_limits<uint32_t>::max();
	};
	class DLLRTUTIL SceneObject : public BaseObject {
	  public:
		virtual ~SceneObject() = default;
		Scene &GetScene() const;
	  protected:
		SceneObject(Scene &scene);
	  private:
		Scene &m_scene;
	};
};
