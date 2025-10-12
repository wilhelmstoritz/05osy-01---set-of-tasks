#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <vector>
#include <string>

// check if a file is valid for reading
bool is_valid_file(const char* t_filepath) {
    struct stat file_stat;
    
    // ... file exists and get stats
    if (stat(t_filepath, &file_stat) != 0) {
        fprintf(stderr, "[error] cannot stat file '%s'\n", t_filepath);
        return false;
    }
    
    // ... it's a directory
    if (S_ISDIR(file_stat.st_mode)) {
        fprintf(stderr, "[info]  skipping directory '%s'\n", t_filepath);
        return false;
    }
    
    // ... it's executable (we want to skip executables)
    if (file_stat.st_mode & S_IXUSR) {
        fprintf(stderr, "[info]  skipping executable '%s'\n", t_filepath);
        return false;
    }
    
    // ... it's readable
    if (access(t_filepath, R_OK) != 0) {
        fprintf(stderr, "[error] cannot read file '%s'\n", t_filepath);
        return false;
    }
    
    return true;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s <file1> [file2] ...\n", argv[0]);
        fprintf(stderr, "example: %s *.txt\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    std::vector<std::string> valid_files;

    // arguments
    printf("--- processing %d argument(s) ------------\n", argc - 1);
    for (int i = 1; i < argc; i++) {
        //printf("checking: '%s'\n", argv[i]);
        
        if (is_valid_file(argv[i])) {
            valid_files.push_back(argv[i]);
            //printf(" > VALID: added to processing list\n");
        } else {
            //printf(" > SKIPPED\n");
        }
        //printf("\n");
    }
    
    // summary
    printf("\n--- summary -----------------------------\n");
    printf("total arguments: %d\n", argc - 1);
    printf("valid files: %lu\n", valid_files.size());
    printf("skipped: %d\n", argc - 1 - (int)valid_files.size());
    
    if (valid_files.empty()) {
        fprintf(stderr, "[error] no valid files to process\n");
        return EXIT_FAILURE;
    }
    
    printf("\n--- valid files for processing ----------\n");
    for (size_t i = 0; i < valid_files.size(); i++) {
        printf("%lu. %s\n", i + 1, valid_files[i].c_str());
    }
    
    return EXIT_SUCCESS;
}
