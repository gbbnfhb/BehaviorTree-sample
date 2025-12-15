-- find_random_location.lua

FindRandomLocation = {}
FindRandomLocation.__index = FindRandomLocation

-- コンストラクタ：探索半径と行動範囲を受け取る
function FindRandomLocation:new(search_radius, bounds)
    local obj = {
        search_radius = search_radius or 300.0,
        bounds = bounds
    }
    setmetatable(obj, self)
    return obj
end

-- tick関数：引数は agent と opponent のみ
function FindRandomLocation:tick(agent, opponent)
--print "FindRandomLocation:tick"
    -- agentオブジェクトを経由してblackboardを取得
    local blackboard = agent.blackboard

    if blackboard:has("target_location") then
        return "SUCCESS"
    end

    local current_pos = agent.position
    
    -- 新しい目標地点を計算
    local angle = math.random() * 2 * math.pi
    local distance = math.random() * self.search_radius
    local target_x = current_pos.x + math.cos(angle) * distance
    local target_y = current_pos.y + math.sin(angle) * distance

    -- 計算した目標地点を、指定された範囲(bounds)内に収める
    if self.bounds then
        target_x = math.max(self.bounds.min_x, math.min(target_x, self.bounds.max_x))
        target_y = math.max(self.bounds.min_y, math.min(target_y, self.bounds.max_y))
    end

    local new_target = Vector2.new(target_x, target_y)

    -- agent経由で取得したblackboardに書き込む
    blackboard:set_vector2("target_location", new_target)
    
    return "SUCCESS"
end