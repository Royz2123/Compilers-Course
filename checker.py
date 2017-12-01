import difflib
import sys

"""
A simple program that compares all the files in a directory, and outputs the ones
that are similar above a certain THRESHOLD
"""


# function that prints out the correct usage of the program
def print_usage():
    print """
        Usage: checker.py   dir   [THRESHOLD = 0] 
    """
    
# function that returns a number that represents the difference between two files
def compare_files(file1, file2):
    with open(file1, "r") as f1:
        with open(file2, "r") as f2:
            file1_lines = f1.read().split('\n')
            file2_lines = f2.read().split('\n')
            
           
           
# function that displays a list of similar files
def display_files(file_list):
    # for now just print the filenames
    for files in file_list:
        print("%d, %d" % (files[0], files[1]))
    
    
def main():
    if len(sys.argv) < 2:
        print_usage()
        return
    
    # find directory name
    dir_name = sys.argv[1]
    
    # decide on threshold that is considered displayable
    chosen_thresh = DEFAULT_THRESHOLD
    if len(sys.argv) >= 3:
        chosen_thresh = int(sys.argv[2])
    
    # loop through all the files in the chosen directory
    sim_files = []
    for file1 in os.listdir(dir_name):
        for file2 in os.listdir(dir_name):
            # make sure we don't compare a file to itself
            if file1 == file2:
                continue
    
            if compare_files(file1, file2) >= THRESHOLD:
                sim_files.append((file1, file2))
    
    # display similar files
    display_files(sim_files)
    
    return
   
   
if __name__ == "__main__":
    main()

