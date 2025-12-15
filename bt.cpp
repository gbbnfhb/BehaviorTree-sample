#include "raylib.h"
#include <vector>
#include <string>
#include <cmath>
#include <memory>
#include <iostream>

#define SOL_ALL_SAFETIES_ON 1
#include "sol3/sol.hpp"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

// --- 前方宣言 ---
class Agent;

// --- ビヘイビアツリー基本コンポーネント ---
enum class NodeStatus { SUCCESS, FAILURE, RUNNING };

std::string StatusToString(NodeStatus status) {
	switch (status) {
	case NodeStatus::SUCCESS: return "SUCCESS";
	case NodeStatus::FAILURE: return "FAILURE";
	case NodeStatus::RUNNING: return "RUNNING";
	}
	return "UNKNOWN";
}

class Node {
protected:
	NodeStatus status = NodeStatus::FAILURE;
public:
	virtual ~Node() = default;
	virtual NodeStatus tick(Agent& agent, const Agent& opponent) = 0;
	virtual std::string getStatusText() const = 0;
	virtual void getTreeViewText(std::string& text, const std::string& indent) const {
		text += indent + getStatusText() + "\n";
	}
};

class CompositeNode : public Node {
protected:
	std::vector<std::shared_ptr<Node>> children;
public:
	CompositeNode(std::vector<std::shared_ptr<Node>> children_nodes) : children(std::move(children_nodes)) {}
	void getTreeViewText(std::string& text, const std::string& indent) const override {
		text += indent + getStatusText() + "\n";
		for (const auto& child : children) {
			child->getTreeViewText(text, indent + "  ");
		}
	}
};

class Sequence : public CompositeNode {
public:
	using CompositeNode::CompositeNode;
	NodeStatus tick(Agent& agent, const Agent& opponent) override {
		for (const auto& child : children) {
			NodeStatus child_status = child->tick(agent, opponent);
			if (child_status != NodeStatus::SUCCESS) {
				status = child_status;
				return status;
			}
		}
		status = NodeStatus::SUCCESS;
		return status;
	}
	std::string getStatusText() const override { return "Sequence: " + StatusToString(status); }
};

// --- Agentクラス ---
class Agent {
public:
	Vector2 position;
	Color color;
	float speed = 1.0f;
	std::shared_ptr<Node> behavior_tree = nullptr;

	Agent(float x, float y, Color c, std::shared_ptr<Node> bt = nullptr)
		: position{x, y}, color(c), behavior_tree(std::move(bt)) {}

	void update(const Agent& opponent) {
		if (behavior_tree) {
			behavior_tree->tick(*this, opponent);
		}
	}

	void draw() const {
		DrawCircleV(position, 15.0f, color);
	}
};

// --- Lua連携ノード ---
class LuaNode : public Node {
private:
	sol::state_view lua;
	sol::table self; // Lua側のノードインスタンス
	std::string node_name;

public:
	LuaNode(sol::state_view lua_vm, const std::string& script_path, const std::string& class_name, sol::variadic_args args)
		: lua(lua_vm), node_name(class_name)
	{
		try {
			lua.script_file(script_path);
			sol::table node_class = lua[class_name];
			// :new(self, ...)の形で呼び出す
			self = node_class["new"](node_class, args);
		} catch (const sol::error& e) {
			std::cerr << "Lua Error in LuaNode constructor: " << e.what() << std::endl;
			self = sol::nil;
		}
	}

	NodeStatus tick(Agent& agent, const Agent& opponent) override {
		if (!self.valid()) return NodeStatus::FAILURE;
		try {
			sol::object result = self["tick"](self, std::ref(agent), std::ref(opponent));
			if (result.is<std::string>()) {
				std::string status_str = result.as<std::string>();
				if (status_str == "SUCCESS") { status = NodeStatus::SUCCESS; return status; }
				if (status_str == "RUNNING") { status = NodeStatus::RUNNING; return status; }
			}
		} catch (const sol::error& e) {
			std::cerr << "Lua Error in tick: " << e.what() << std::endl;
		}
		status = NodeStatus::FAILURE;
		return status;
	}

	// getStatusTextとgetTreeViewTextを実装する
	std::string getStatusText() const override {
		if (!self.valid()) return node_name + ": [INVALID]";
		try {
			// Lua側のgetStatusTextを呼び出す
			sol::function get_status = self["getStatusText"];
			if (get_status.valid()) {
				return get_status(self).get<std::string>();
			}
		} catch (const sol::error& e) {
			std::cerr << "Lua Error in getStatusText: " << e.what() << std::endl;
		}
		return node_name + ": " + StatusToString(status);
	}
};


// --- メインプログラム ---
int main() {
	const int SCREEN_WIDTH = 800;
	const int SCREEN_HEIGHT = 600;
	const int GUI_WIDTH = 250;

	InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Behavior Tree with Lua - C++");
	SetTargetFPS(60);

	// --- Luaのセットアップ ---
	sol::state lua;
	lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string);

	// Vector2のコンストラクタの代わりにファクトリ関数を登録
	lua.new_usertype<Vector2>("Vector2",
		sol::factories([](float x, float y) { return Vector2{x, y}; }),
		"x", &Vector2::x,
		"y", &Vector2::y
	);

	// AgentクラスをLuaに公開
	lua.new_usertype<Agent>("Agent",
		sol::no_constructor, // C++側で生成するのでLuaでのコンストラクタは不要
		"position", &Agent::position,
		"speed", &Agent::speed,
		// LuaからC++のAgentを移動させるためのメソッド
		"move", [](Agent& agent, float dx, float dy) {
		agent.position.x += dx;
		agent.position.y += dy;
	}
	);

	// --- アバターとビヘイビアツリーの作成 ---
	Agent player(100, SCREEN_HEIGHT / 2.0f, MAROON);

	auto ai_tree = std::make_shared<Sequence>(std::vector<std::shared_ptr<Node>>{
		std::make_shared<LuaNode>(lua, "scripts/is_enemy_close.lua", "IsEnemyClose", sol::variadic_args(lua, 200.0f)),
			std::make_shared<LuaNode>(lua, "scripts/move_to_enemy.lua", "MoveToEnemy", sol::variadic_args(lua))
	});

	Agent ai(SCREEN_WIDTH - 100 - GUI_WIDTH, SCREEN_HEIGHT / 2.0f, DARKBLUE, ai_tree);

	std::string tree_view_text;

	while (!WindowShouldClose()) {
		player.position = GetMousePosition();
		if (player.position.x > SCREEN_WIDTH - GUI_WIDTH) {
			player.position.x = SCREEN_WIDTH - GUI_WIDTH;
		}

		ai.update(player);

		tree_view_text.clear();
		ai_tree->getTreeViewText(tree_view_text, "");

		BeginDrawing();
		ClearBackground(RAYWHITE);
		DrawLine(SCREEN_WIDTH - GUI_WIDTH, 0, SCREEN_WIDTH - GUI_WIDTH, SCREEN_HEIGHT, LIGHTGRAY);
		player.draw();
		ai.draw();
		DrawCircleLines(static_cast<int>(ai.position.x), static_cast<int>(ai.position.y), 200.0f, Fade(SKYBLUE, 0.5f));

		GuiGroupBox({(float)SCREEN_WIDTH - GUI_WIDTH + 5, 10, (float)GUI_WIDTH - 15, (float)SCREEN_HEIGHT - 20}, "AI Behavior Tree Status");
		GuiLabel({(float)SCREEN_WIDTH - GUI_WIDTH + 15, 40, (float)GUI_WIDTH - 30, (float)SCREEN_HEIGHT - 50}, tree_view_text.c_str());

		EndDrawing();
	}

	CloseWindow();
	return 0;
}


