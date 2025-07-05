// SPDX-FileCopyrightText: (c) 2024 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "definitions.hpp"
#include <exception>
#include <string>

export module pragma.scenekit:exception;

export namespace pragma::scenekit {
	class DLLRTUTIL Exception : public std::exception {
	  public:
		Exception(const std::string &msg) : m_message {msg} {}
		virtual char const *what() const noexcept override { return m_message.c_str(); }
	  private:
		std::string m_message {};
	};
};
