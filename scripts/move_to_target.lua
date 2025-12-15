-- move_to_target.lua

MoveToTarget = {}
MoveToTarget.__index = MoveToTarget

function MoveToTarget:new()
    local obj = {}
    setmetatable(obj, self)
    return obj
end

function MoveToTarget:tick(agent, opponent)
--print "MoveToTarget:tick"
    -- ブラックボードに目標地点がなければ、何もできないので「失敗(FAILURE)」
    if not agent.blackboard:has("target_location") then
        return "FAILURE"
    end

    -- ブラックボードから目標地点を読み込む
    local target_pos = agent.blackboard:get_vector2("target_location")
    local current_pos = agent.position

    -- 目標地点と現在位置の差（ベクトル）を計算
    local diff_x = target_pos.x - current_pos.x
    local diff_y = target_pos.y - current_pos.y
    
    -- 距離を計算
    local distance = math.sqrt(diff_x * diff_x + diff_y * diff_y)

    -- もし目標に十分に近づいていたら（例: 5ピクセル以内）
    if distance < 5.0 then
        -- 目標をブラックボードから削除する
        agent.blackboard:remove("target_location")
        -- 移動が完了したので「成功(SUCCESS)」を返す
        -- print("Arrived at target.")
        return "SUCCESS"
    end

    -- まだ到着していない場合
    -- 移動方向の単位ベクトルを計算（正規化）
    local dir_x = diff_x / distance
    local dir_y = diff_y / distance

    -- エージェントの位置を更新する
    agent.position.x = current_pos.x + dir_x * agent.speed
    agent.position.y = current_pos.y + dir_y * agent.speed

    -- まだ移動中なので「実行中(RUNNING)」を返す
    return "RUNNING"
end