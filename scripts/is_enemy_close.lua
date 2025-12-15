-- scripts/is_enemy_close.lua
print("--- Loading script: is_enemy_close.lua ---")

IsEnemyClose = {}
IsEnemyClose.__index = IsEnemyClose

function IsEnemyClose:new(threshold)
  local self = setmetatable({}, IsEnemyClose)
  self.distance_threshold = threshold or 200.0
  self.status = "FAILURE" 
  return self
end

function IsEnemyClose:tick(agent, opponent)
  local p1 = agent.position
  local p2 = opponent.position
  
  local dx = p1.x - p2.x
  local dy = p1.y - p2.y
  local distance = math.sqrt(dx*dx + dy*dy)
  
  if distance < self.distance_threshold then
    self.status = "SUCCESS"
  else
    self.status = "FAILURE"
  end
  
  return self.status
end

function IsEnemyClose:getStatusText()
    return string.format("IsEnemyClose(<%d): %s", self.distance_threshold, self.status)
end