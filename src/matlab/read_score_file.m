function output = read_score_file(file_name, from_time, to_time)

    file = fopen(file_name, 'r', 'b');

    time = fread(file, Inf, 'int64=>int64', 8);
    frewind(file);

    fread(file, 1, 'int64=>int64');
    score = fread(file, Inf, 'int64=>int64', 8);
    
    modified_score = score(1);
    modified_time = time(1);

    for original_index = 2 : min([length(score) length(time)])

        if score(original_index - 1) == 0
            modified_score = [modified_score 0];
            modified_time = [modified_time time(original_index)];
        end

        if score(original_index - 1) == 5
            modified_score = [modified_score 5];
            modified_time = [modified_time time(original_index)];
        end

        modified_score = [modified_score score(original_index)];
        modified_time = [modified_time time(original_index)];
    end
    
    index = find((modified_time >= from_time) & (modified_time <= to_time));
    
    output = [modified_time(index); modified_score(index)];

end