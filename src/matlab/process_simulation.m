function utilizations = process_simulation(folder_name, max_score, max_clients)

    addpath(folder_name);
    
    utilizations = [];
    
    files = dir(folder_name);
    
    for index = 1 : length(files)
        
        file_name = files(index);
        
        if length(file_name.name) > 3
            
            if strcmp(file_name.name(end - 3:end), '.csv')

                utilizations(end + 1) = calculate_utilization(file_name.name, max_score, max_clients);

            end
        
        end
        
    end

end