function utilization = calculate_utilization(file_name, max_score, max_clients)

    master_data = readmatrix(file_name);
    
    from_time = master_data(1,1);
    to_time = master_data(end,1);
    
    utilization = double(0);

    for client_index = 1 : max_clients

        client_file_name = ['client-' num2str(client_index) '_time'];
        
        client_data = read_score_file(client_file_name, from_time, to_time);

        for index = 1 : length(client_data) - 1

            delta_time = client_data(1, index + 1) - client_data(1, index);

            if client_data(2, index) == max_score
                utilization = utilization + delta_time;
            end

        end

    end

    utilization = double(utilization) / double((to_time - from_time) * max_clients);
    
end