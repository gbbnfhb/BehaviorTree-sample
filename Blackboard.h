#include <string>
#include <unordered_map>
#include <any> // C++17以降。様々な型を格納できる
#include "raylib.h" // Vector2のためにインクルード

// ブラックボードクラス
class Blackboard {
private:
    // 文字列をキーとして、任意の型のデータを保持するマップ
    std::unordered_map<std::string, std::any> data;

public:
    // データを書き込む（任意の型に対応）
    template<typename T>
    void set(const std::string& key, T value) {
        data[key] = value;
    }

    // データを読み込む（型を指定して取得）
    template<typename T>
    T get(const std::string& key) const {
        // キーが存在し、かつ型が一致する場合
        if (data.count(key) && data.at(key).type() == typeid(T)) {
            return std::any_cast<T>(data.at(key));
        }
        // 存在しない、または型が違う場合はデフォルト値を返す
        return T{};
    }

    // キーが存在するかチェック
    bool has(const std::string& key) const {
        return data.count(key) > 0;
    }

    // キーを削除する
    void remove(const std::string& key) {
        data.erase(key);
    }
};