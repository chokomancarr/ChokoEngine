#pragma once
#include "Engine.h"

/*! 2D drawing to the screen.
[av] */
class UI {
public:
	static void Texture(float x, float y, float w, float h, ::Texture* texture, DRAWTEX_SCALING scl = DRAWTEX_STRETCH, float miplevel = 0);
	static void Texture(float x, float y, float w, float h, ::Texture* texture, float alpha, DRAWTEX_SCALING scl = DRAWTEX_STRETCH, float miplevel = 0);
	static void Texture(float x, float y, float w, float h, ::Texture* texture, Vec4 tint, DRAWTEX_SCALING scl = DRAWTEX_STRETCH, float miplevel = 0);
	static void Label(float x, float y, float s, string str, Font* font, Vec4 col = black(), float maxw = -1);

	//Draws an editable text box. EditText does not work on recursive functions.
	static string EditText(float x, float y, float w, float h, float s, Vec4 bcol, const string& str, Font* font, bool delayed = false, bool* changed = nullptr, Vec4 fcol = black(), Vec4 hcol = Vec4(0, 120.0f / 255, 215.0f / 255, 1), Vec4 acol = white(), bool ser = true);

	static bool CanDraw();

	static bool _isDrawingLoop;
	static void PreLoop();
	static uintptr_t _activeEditText[UI_MAX_EDIT_TEXT_FRAMES], _lastEditText[UI_MAX_EDIT_TEXT_FRAMES], _editingEditText[UI_MAX_EDIT_TEXT_FRAMES];
	static ushort _activeEditTextId, _editingEditTextId;

	static void GetEditTextId();
	static bool IsActiveEditText();
	static bool IsSameId(uintptr_t* left, uintptr_t* right);

	struct StyleColor {
		Vec4 backColor, fontColor;
		//Texture* backTex;

		void Set(const Vec4 vb, const Vec4 vf) {
			backColor = vb;
			fontColor = vf;
		}
	};
	struct Style {
		StyleColor normal, mouseover, highlight, press;
		int fontSize;
	};

	friend class Engine;
	friend void FocusGL(GLFWwindow* window, int focus);
	friend class PopupSelector;
	friend class RenderTexture;
	//protected:

	static bool focused;
	static uint _editTextCursorPos, _editTextCursorPos2;
	static string _editTextString;
	static float _editTextBlinkTime;

	static Style _defaultStyle;

	static void InitVao(), SetVao(uint sz, void* verts, void* uvs = nullptr);
	static uint _vboSz;
	static GLuint _vao, _vboV, _vboU;

	static void Init();
};