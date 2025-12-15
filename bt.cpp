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

#include "bt.h"

void DrawBehaviorTree(const Node* node, int x, int& y, int indent_level) {
	if (!node) return;

	// インデントをつけて表示
	int indent = indent_level * 20;

	// ノードのステータスに応じて色分けする
	Color textColor = WHITE;
	switch (node->getStatus()) { // getStatus()も必要
	case NodeStatus::SUCCESS: textColor = GREEN; break;
	case NodeStatus::FAILURE: textColor = RED; break;
	case NodeStatus::RUNNING: textColor = BLUE; break;
	default: textColor = GRAY; break;
	}

	// ノードの名前とステータスを描画
	DrawText(node->getDebugText().c_str(), x + indent, y, 10, textColor);

	// y座標を次の行へ
	y += 15;

	// 子ノードを再帰的に描画
	for (const auto& child : node->getChildren()) {
		DrawBehaviorTree(child.get(), x, y, indent_level + 1);
	}
}

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

	lua.new_usertype<Blackboard>("Blackboard",
		"set_vector2", &Blackboard::set<Vector2>, // Vector2専用のsetを登録
		"get_vector2", &Blackboard::get<Vector2>, // Vector2専用のgetを登録
		"has", &Blackboard::has,
		"remove", &Blackboard::remove
	);

	// AgentクラスをLuaに公開
	lua.new_usertype<Agent>("Agent",
		sol::no_constructor, // C++側で生成するのでLuaでのコンストラクタは不要
		"position", &Agent::position,
		"blackboard", &Agent::blackboard,
		"speed", &Agent::speed,
		// LuaからC++のAgentを移動させるためのメソッド
		"move", [](Agent& agent, float dx, float dy) {
		agent.position.x += dx;
		agent.position.y += dy;
	}
	);



	// --- アバターとビヘイビアツリーの作成 ---
	Agent player(100, SCREEN_HEIGHT / 2.0f, MAROON);

	// 【優先度:高】の行動ブランチ（シーケンス）を作成
	// これは「もし敵が近ければ、敵に向かって移動する」という一連の行動
	auto chase_sequence = std::make_shared<Sequence>(std::vector<std::shared_ptr<Node>>{
		// 条件：敵が200ピクセル以内にいるか？
		std::make_shared<LuaNode>(lua, "scripts/is_enemy_close.lua", "IsEnemyClose", 
			200.0f),
			// 行動：敵に向かって移動する
			std::make_shared<LuaNode>(lua, "scripts/move_to_enemy.lua", "MoveToEnemy")
	});

	//行動範囲
	sol::table bounds = lua.create_table_with(
		"min_x", 0.0f,
		"min_y", 0.0f,
		"max_x", (float)SCREEN_WIDTH - GUI_WIDTH,
		"max_y", (float)SCREEN_HEIGHT
	);

	// ランダム徘徊シーケンスを作成
	auto wander_sequence = std::make_shared<Sequence>(std::vector<std::shared_ptr<Node>>{
		// 1. 行き先を探すノード
		std::make_shared<LuaNode>(lua, 
			"scripts/find_random_location.lua",	"FindRandomLocation", 
			300.0f,
			bounds), // 300.0fは探索半径
		// 2. そこへ移動するノード
		std::make_shared<LuaNode>(lua, "scripts/move_to_target.lua", "MoveToTarget")
	});

	// セレクターを使って、2つの行動ブランチをまとめる
	// これがビヘイビアツリー全体のルートノードになる
	auto root_selector = std::make_shared<Selector>(std::vector<std::shared_ptr<Node>>{
		chase_sequence, // 最初にこちらを試す
			wander_sequence   // 上が失敗したら、こちらを実行する
	});


	Agent ai(SCREEN_WIDTH - 100 - GUI_WIDTH, SCREEN_HEIGHT / 2.0f, DARKBLUE, root_selector);

	std::string tree_view_text;

	while (!WindowShouldClose()) {
		player.position = GetMousePosition();
		if (player.position.x > SCREEN_WIDTH - GUI_WIDTH) {
			player.position.x = SCREEN_WIDTH - GUI_WIDTH;
		}

		ai.update(player);

		tree_view_text.clear();
		root_selector->getTreeViewText(tree_view_text, "");

		BeginDrawing();
		ClearBackground(RAYWHITE);
		DrawLine(SCREEN_WIDTH - GUI_WIDTH, 0, SCREEN_WIDTH - GUI_WIDTH, SCREEN_HEIGHT, LIGHTGRAY);
		player.draw();
		ai.draw();
		DrawCircleLines(static_cast<int>(ai.position.x), static_cast<int>(ai.position.y), 200.0f, Fade(SKYBLUE, 0.5f));

		GuiGroupBox({(float)SCREEN_WIDTH - GUI_WIDTH + 5, 10, (float)GUI_WIDTH - 15, (float)SCREEN_HEIGHT - 20}, "AI Behavior Tree Status");
//		GuiLabel({(float)SCREEN_WIDTH - GUI_WIDTH + 15, 40, (float)GUI_WIDTH - 30, (float)SCREEN_HEIGHT - 50}, tree_view_text.c_str());
		// GUI領域の背景を描画
		DrawRectangle(SCREEN_WIDTH - GUI_WIDTH, 0, GUI_WIDTH, SCREEN_HEIGHT, Fade(LIGHTGRAY, 0.5f));

		// ビヘイビアツリーのデバッグ表示
		int start_y = 50;
		DrawBehaviorTree(ai.behavior_tree.get(), SCREEN_WIDTH - GUI_WIDTH + 10, start_y, 0);
		EndDrawing();
	}

	CloseWindow();
	return 0;
}


