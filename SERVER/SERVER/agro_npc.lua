myid = 99999;
target_player_id = -1;

move_dx = 0;
move_dy = 0;

agro_range = 11

function set_uid(x)
    myid = x;
end

function event_player_move(player)
    player_x = API_get_x(player);
    if (-1 == player_x) then
        target_player_id = -1;
        return target_player_id;
    end

    player_y = API_get_y(player);
    if (-1 == player_y) then
        target_player_id = -1;
        return target_player_id;
    end

    my_x = API_get_x(myid);
    my_y = API_get_y(myid);
    if (math.abs(player_x - my_x) < agro_range) then
        if (math.abs(player_y - my_y) < agro_range) then
            target_player_id = player;
        else 
            target_player_id = -1;
        end
    else
        target_player_id = -1;
    end

    return target_player_id;
end