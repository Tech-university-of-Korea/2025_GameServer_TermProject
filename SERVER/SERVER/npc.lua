myid = 99999;
state_collision_with_player = false;
collision_player_id = 99999;
random_move_turn = 0;

move_dx = 0;
move_dy = 0;

function set_uid(x)
    myid = x;
end

function event_do_npc_random_move()
    if (true == state_collision_with_player and 99999 ~= collision_player_id) then
        local player_x = API_get_x(collision_player_id);
        local player_y = API_get_y(collision_player_id);
        my_x = API_get_x(myid);
        my_y = API_get_y(myid);

        local diff_x = my_x - player_x;
        local diff_y = my_y - player_y;

        if (diff_x == 0) then
            move_dx = 0;
        else
            move_dx = diff_x / math.abs(diff_x);
        end

        if (diff_y == 0) then
            move_dy = 0;
        else
            move_dy = diff_y / math.abs(diff_y);
        end

        if (0 == move_dx and 0 == move_dy) then
            move_dx = -1;
            move_dy = -1;
        end

        if (random_move_turn > 0) then
            API_send_message(myid, collision_player_id, "HELLO");
            random_move_turn = random_move_turn - 1;
        elseif (random_move_turn == 0) then
            API_send_message(myid, collision_player_id, "BYE");
            state_collision_with_player = false;
        end
    end
end

function event_player_move(player)
    player_x = API_get_x(player);
    player_y = API_get_y(player);
    my_x = API_get_x(myid);
    my_y = API_get_y(myid);
    if (player_x == my_x) then
        if (player_y == my_y) then
            state_collision_with_player = true;
            collision_player_id = player;
            random_move_turn = 3;
        end
    end
end