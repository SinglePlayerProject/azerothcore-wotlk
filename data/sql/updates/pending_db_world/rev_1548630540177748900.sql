INSERT INTO version_db_world (`sql_rev`) VALUES ('1548630540177748900');

-- Removing double quest from creature Captain Hecklebury Smotts
DELETE FROM `creature_queststarter` WHERE `id`=2500 AND `quest`=615;

-- Creatures faction to 67 - Horde
UPDATE `creature_template` SET `faction`='67' WHERE `entry`=12858;
UPDATE `creature_template` SET `faction`='67' WHERE `entry`=12859;

-- NPC Sarkoth faction - Monster
UPDATE `creature_template` SET `faction`='14' WHERE `entry`=3281;

-- Pathing for Centaur Pariah
UPDATE `creature` SET `position_x`=-2145.71, `position_y`=1966.42, `position_z`=84.4919, `spawndist`=0, `MovementType`=2 WHERE `guid`=29069;

DELETE FROM `creature_addon` WHERE `guid`=29069;
INSERT INTO `creature_addon` (`guid`, `path_id`, `mount`, `bytes1`, `bytes2`, `emote`, `auras`) VALUES
(29069,290690,0,0,1,0,"");

DELETE FROM `waypoint_data` WHERE `id`=290690;
INSERT INTO `waypoint_data` (`id`, `point`, `position_x`, `position_y`, `position_z`, `orientation`, `delay`, `move_type`, `action`, `action_chance`, `wpguid`) VALUES
(290690,1,-2145.71,1966.42,84.4919,0,0,0,0,100,0),
(290690,2,-2148.35,1973.16,84.0701,0,0,0,0,100,0),
(290690,3,-2156.93,1972.24,80.9217,0,0,0,0,100,0),
(290690,4,-2158.67,1959.92,78.2252,0,0,0,0,100,0),
(290690,5,-2161.1,1953.68,76.6696,0,0,0,0,100,0),
(290690,6,-2159.54,1948.01,74.4792,0,0,0,0,100,0),
(290690,7,-2166.39,1942.92,69.9358,0,0,0,0,100,0),
(290690,8,-2165.94,1938.13,66.2389,0,0,0,0,100,0),
(290690,9,-2167.71,1935.14,63.539,0,0,0,0,100,0),
(290690,10,-2173.82,1935.16,61.1357,0,0,0,0,100,0),
(290690,11,-2185.89,1953.4,61.0883,0,0,0,0,100,0),
(290690,12,-2181.04,1971.95,63.0648,0,0,0,0,100,0),
(290690,13,-2185.23,1996.62,64.0418,0,0,0,0,100,0),
(290690,14,-2184.85,2019.55,64.0418,0,0,0,0,100,0),
(290690,15,-2182.58,2034.38,64.3299,0,0,0,0,100,0),
(290690,16,-2187.89,2045.21,65.1992,0,0,0,0,100,0),
(290690,17,-2175.83,2065.84,63.6989,0,0,0,0,100,0),
(290690,18,-2165.2,2087.16,64.4523,0,0,0,0,100,0),
(290690,19,-2156.65,2104.11,61.6077,0,0,0,0,100,0),
(290690,20,-2151.67,2118.24,60.7861,0,0,0,0,100,0),
(290690,21,-2148.89,2129.81,63.6364,0,0,0,0,100,0),
(290690,22,-2144.39,2148.28,65.8843,0,0,0,0,100,0),
(290690,23,-2141.23,2166.66,66.9008,0,0,0,0,100,0),
(290690,24,-2139.35,2184.03,66.5107,0,0,0,0,100,0),
(290690,25,-2134.91,2204.03,65.2096,0,0,0,0,100,0),
(290690,26,-2131.41,2219.61,64.1073,0,0,0,0,100,0),
(290690,27,-2128.85,2237.09,64.7163,0,0,0,0,100,0),
(290690,28,-2128.27,2222.75,63.9388,0,0,0,0,100,0),
(290690,29,-2133.74,2204.02,65.2261,0,0,0,0,100,0),
(290690,30,-2137.84,2186.67,66.0619,0,0,0,0,100,0),
(290690,31,-2141.6,2170.8,67.1661,0,0,0,0,100,0),
(290690,32,-2147.93,2144.09,66.0683,0,0,0,0,100,0),
(290690,33,-2152.54,2124.61,62.9209,0,0,0,0,100,0),
(290690,34,-2154.5,2116.3,61.1993,0,0,0,0,100,0),
(290690,35,-2160.27,2091.94,63.4087,0,0,0,0,100,0),
(290690,36,-2171.46,2076.47,64.0607,0,0,0,0,100,0),
(290690,37,-2180,2055.74,63.8618,0,0,0,0,100,0),
(290690,38,-2187.17,2043.48,65.118,0,0,0,0,100,0),
(290690,39,-2182.03,2025.33,64.3237,0,0,0,0,100,0),
(290690,40,-2185.91,2006.41,64.0427,0,0,0,0,100,0),
(290690,41,-2184.03,1990.34,64.0427,0,0,0,0,100,0),
(290690,42,-2182.62,1974.82,63.3536,0,0,0,0,100,0),
(290690,43,-2185.55,1962.21,62.0913,0,0,0,0,100,0),
(290690,44,-2182.01,1945.01,60.5147,0,0,0,0,100,0),
(290690,45,-2174.77,1934.49,61.1065,0,0,0,0,100,0),
(290690,46,-2165.66,1935.02,64.5662,0,0,0,0,100,0),
(290690,47,-2166.1,1945.12,71.6517,0,0,0,0,100,0),
(290690,48,-2159.56,1948.89,74.686,0,0,0,0,100,0),
(290690,49,-2159.53,1958.33,77.9476,0,0,0,0,100,0),
(290690,50,-2156.65,1975.9,81.5158,0,0,0,0,100,0),
(290690,51,-2145.71,1971.54,84.3149,0,0,0,0,100,0),
(290690,52,-2143.66,1964.77,84.0694,0,0,0,0,100,0);
