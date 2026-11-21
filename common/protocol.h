#ifndef PROTOCOL_H
#define PROTOCOL_H

/* Cau hinh mang & buffer */
#define DEFAULT_PORT 9999
#define BUF_SIZE     4096
#define MAX_CLIENTS  64

/* Thong so phi thuyen & doi choi */
#define MAX_TEAM_SIZE    3
#define INIT_HP          1000
#define INIT_ARMOR       0
#define INIT_AMMO_30MM   50

#define MAX_CANNONS      4
#define MAX_MISSILES     4
#define MAX_ARMOR_LAYERS 2

/* Vu khi, giap, ruong & sua chua */
#define DAM_30MM             10
#define PRICE_AMMO_30MM      100
#define AMMO_CRATE_SIZE      50

#define DAM_LASER            100
#define PRICE_LASER          1000
#define PRICE_LASER_POWER    100
#define LASER_POWER_SHOTS    10

#define DAM_MISSILE          800
#define PRICE_MISSILE        2000

#define ARMOR_BASIC_VAL      500
#define PRICE_ARMOR_BASIC    1000
#define ARMOR_ENHANCED_VAL   1500
#define PRICE_ARMOR_ENHANCED 2000

#define CHEST_BRONZE_COIN    100
#define CHEST_SILVER_COIN    500
#define CHEST_GOLD_COIN      2000

#define REPAIR_COST_PER_HP   1

/* Cac lenh co ban */
#define CMD_PING     "PING"
#define CMD_REGISTER "REGISTER"
#define CMD_LOGIN    "LOGIN"
#define CMD_LOGOUT   "LOGOUT"
#define CMD_WHOAMI   "WHOAMI"

/* Lenh quan ly doi */
#define CMD_TEAM_CREATE  "TEAM_CREATE"

#endif
