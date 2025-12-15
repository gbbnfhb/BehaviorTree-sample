-- scripts/move_to_enemy.lua
print("--- Loading script: move_to_enemy.lua ---")

MoveToEnemy = {}
MoveToEnemy.__index = MoveToEnemy

function MoveToEnemy:new()
  local self = setmetatable({}, MoveToEnemy)
  self.status = "FAILURE"
  return self
end

function MoveToEnemy:tick(agent, opponent)
--print "MoveToEnemy:tick"
  local p1 = agent.position
  local p2 = opponent.position
  
  local dx = p2.x - p1.x
  local dy = p2.y - p1.y
  local dist = math.sqrt(dx*dx + dy*dy)

  -- 敵との距離が、1フレームで進む距離より大きいかチェック
  if dist > agent.speed then
    -- 距離がまだある場合：敵に向かって移動する
    -- C++で登録したagent:move()を呼び出す
    agent:move((dx / dist) * agent.speed, (dy / dist) * agent.speed)
    self.status = "RUNNING"
    return "RUNNING" -- "RUNNING"を返す
  else
    -- 敵に到達、または非常に近い場合
    -- ここでは何もしないが、行動は「成功」したとみなす
    self.status = "SUCCESS"
    return "SUCCESS" -- ★★★【修正点】"SUCCESS"を返すのを忘れない！★★★
  end
end

function MoveToEnemy:getStatusText()
    return string.format("MoveToEnemy: %s", self.status)
end