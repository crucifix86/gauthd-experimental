
CREATE TABLE IF NOT EXISTS `arena_players` (

  `player_id` int(11) NOT NULL DEFAULT 0,
  `team_id` int(11) NOT NULL DEFAULT 0,
  `cls` int(11) NOT NULL DEFAULT 0,
  `score` int(11) NOT NULL DEFAULT 0,
  `win_count` int(11) NOT NULL DEFAULT 0,
  `team_score` int(11) NOT NULL DEFAULT 0,
  `week_battle_count` int(11) NOT NULL DEFAULT 0,
  `invite_time` int(11) NOT NULL DEFAULT 0,
  `last_visite` int(11) NOT NULL DEFAULT 0,
  `battle_count` int(11) NOT NULL DEFAULT 0,
  `name` varchar(64) NOT NULL DEFAULT '',
  
  PRIMARY KEY (`player_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;


CREATE TABLE IF NOT EXISTS `arena_teams` (

  `team_id` int(11) AUTO_INCREMENT PRIMARY KEY,
  `captain_id` int(11) NOT NULL DEFAULT 0,
  `team_type` int(11) NOT NULL DEFAULT 0,
  `score` int(11) NOT NULL DEFAULT 0,
  `member1` int(11) NOT NULL DEFAULT 0,
  `member2` int(11) NOT NULL DEFAULT 0,
  `member3` int(11) NOT NULL DEFAULT 0,
  `member4` int(11) NOT NULL DEFAULT 0,
  `member5` int(11) NOT NULL DEFAULT 0,
  `member6` int(11) NOT NULL DEFAULT 0,
  `award1` int(11) NOT NULL DEFAULT 0,
  `award2` int(11) NOT NULL DEFAULT 0,
  `award3` int(11) NOT NULL DEFAULT 0,
  `award4` int(11) NOT NULL DEFAULT 0,
  `award5` int(11) NOT NULL DEFAULT 0,
  `award6` int(11) NOT NULL DEFAULT 0,
  `team3x3` int(11) NOT NULL DEFAULT 0,
  `team6x6` int(11) NOT NULL DEFAULT 0,
  `last_visite` int(11) NOT NULL DEFAULT 0,
  `win_count` int(11) NOT NULL DEFAULT 0,
  `team_score` int(11) NOT NULL DEFAULT 0,
  `week_battle_count` int(11) NOT NULL DEFAULT 0,
  `create_time` int(11) NOT NULL DEFAULT 0,
  `battle_count` int(11) NOT NULL DEFAULT 0,
  `name` varchar(64) NOT NULL DEFAULT ''
  
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
