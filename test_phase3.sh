#!/bin/bash

set -e

# Curăță datele vechi
rm -rf hunt_* monitor_command.txt logged_hunt-* 2>/dev/null || true

# Creează 4 hunt-uri cu denumiri realiste și câte 2 comori
declare -A hunts=(
  [hunt_castle]="Ivana 45.0 25.0 ClueCastle1 100 Bob 46.0 26.0 ClueCastle2 150"
  [hunt_jungle]="Vesna 47.0 27.0 ClueJungle1 200 Alice 48.0 28.0 ClueJungle2 100"
  [hunt_mountain]="Anja 49.0 29.0 ClueMountain1 80 Eve 50.0 30.0 ClueMountain2 120"
  [hunt_desert]="Tijana 51.0 31.0 ClueDesert1 60 Bob 52.0 32.0 ClueDesert2 90"
)

for hunt in "${!hunts[@]}"; do
  mkdir -p "$hunt"
  IFS=' ' read -ra info <<< "${hunts[$hunt]}"
  printf "1\n${info[0]}\n${info[1]}\n${info[2]}\n${info[3]}\n${info[4]}\n" | ./treasure_manager add "$hunt"
  printf "2\n${info[5]}\n${info[6]}\n${info[7]}\n${info[8]}\n${info[9]}\n" | ./treasure_manager add "$hunt"
done

# Rulează comenzi automat prin treasure_hub
{
  echo "start_monitor"
  sleep 1
  echo "list_hunts"
  sleep 1
  echo "list_treasures hunt_jungle"
  sleep 1
  echo "view_treasure hunt_jungle 1"
  sleep 1
  echo "calculate_score"
  sleep 3
  echo "stop_monitor"
  sleep 1
  echo "exit"
} | ./treasure_hub
