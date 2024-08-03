#!/bin/bash


#script will take 4 arguments - submissions, target, tests, answers
#submissions folder will contain zip files
#target folder will contain organized code files
#tests folder will contain test cases
#answers folder will contain answers of the test cases


#check if the submissions folder exists
if [ ! -d "$1" ]; then
    echo "Submissions folder does not exist"
    exit 1
fi

#check if the target folder exists
if [ ! -d "$2" ]; then
    echo "Target folder does not exist"
    exit 1
fi

#check if the tests folder exists
if [ ! -d "$3" ]; then
    echo "Tests folder does not exist"
    exit 1
fi

#check if the answers folder exists
if [ ! -d "$4" ]; then
    echo "Answers folder does not exist"
    exit 1
fi

#check if the submissions folder is empty
if [ -z "$(ls -A $1)" ]; then
    echo "Submissions folder is empty"
    exit 1
fi

#script will take 2 optional arguments - -v and -noexecute
#-v will print organizing and testing confirmation messages
#-noexecute will not execute the code files

#assign the arguments to variables
submissions=$1
targets=$2
tests=$3
answers=$4

#make c,java,python folders under target folder

mkdir -p "$targets/c"
mkdir -p "$targets/java"
mkdir -p "$targets/python"


#extract information from the zipped files of submission folder

for zip_file in "$submissions"/*.zip
do
    if [ -f "$zip_file" ]; then
    # Extract the student's full name from the zip file name
    full_name=$(basename "$zip_file" | cut -d'_' -f1)

    # Extract the student's serial number from the zip file name
     serial_number=$(basename "$zip_file" | cut -d'_' -f2)

    # Extract the student's ID from the zip file name
    student_id=$(basename "$zip_file" | awk -F'_' '{print $NF}' | cut -d'.' -f1)


    # Extract the zip file in a temporary folder
    unzip -q "$zip_file" -d temp

    # Find the code file inside the temp/student_id folder
    # The code file is the file with the extension .c (for C), .java (for Java), or .py (for Python)

    file_code=$(find temp -type f -name "*.c" -o -name "*.java" -o -name "*.py")


    # Determine the programming language of the code file
    # language is the extension of the code file

    language=""

    if [[ $file_code == *.c ]]; then
        language="c"
    elif [[ $file_code == *.java ]]; then
        language="java"
    elif [[ $file_code == *.py ]]; then
        language="python"
    fi

    # Create a new subfolder inside the corresponding language folder

    
    student_folder="$targets/$language/$student_id"
    #echo "Student folder: $student_folder"

    mkdir -p "$student_folder"

    # Copy the code file to the new subfolder and rename it to main.c (for C), Main.java (for Java), or main.py (for Python)
    #if language is c then copy the code file to the new subfolder and rename it to main.c
    #if language is java then copy the code file to the new subfolder and rename it to Main.java
    #if language is python then copy the code file to the new subfolder and rename it to main.py

    if [ "$language" == "c" ]; then
        cp "$file_code" "$student_folder/main.c"
    elif [ "$language" == "java" ]; then
        cp "$file_code" "$student_folder/Main.java"
    elif [ "$language" == "python" ]; then
        cp "$file_code" "$student_folder/main.py"
    fi
    # Clean up the temporary folder

    rm -rf temp


    fi
    if [ "$#" -gt 4 ]; then
    if [ "$5" == "-v" ]; then
        echo "Organizing files of $student_id"
    fi
fi
done




# check if the -noexecute argument is given
# if given then do not execute this part

if [ "$#" -gt 5 ]; then
    if [ "$6" == "-noexecute" ]; then
        exit 1
    fi
fi

# Define the source directory containing the organized code files
source_dir="$targets"

# Define the test cases directory
test_cases_dir="$tests"

# Define the answers directory
answers_dir="$answers"

# Define the result CSV file
result_csv="$source_dir/result.csv"

# Create the result CSV file and write the header
echo "student_id,type,matched,not_matched" > "$result_csv"

# Loop through each extension folder inside the source directory and make object files

for extension_folder in "$source_dir"/*
do
#check if the extension folder is a directory
    if [ ! -d "$extension_folder" ]; then
        continue
    fi
    # Get the extension name
    extension=$(basename "$extension_folder")
    #echo "Extension: $extension"

    # Loop through each student folder inside the extension folder
    for student_folder in "$extension_folder"/*
    do
        # Get the student ID
        student_id=$(basename "$student_folder")
        #echo "Student ID: $student_id"

        echo "Executing files of $student_id"

        # Initialize the matched and not_matched variables
        matched=0
        not_matched=0

        # Loop through each test case file inside the test cases directory
        for test_case_file in "$test_cases_dir"/*
        do
            # Get the output file name
            output_file=$(basename "$test_case_file")
            #echo "Output file: $output_file"

            number=$(echo "$output_file" | cut -d'.' -f1 )
            #echo "$number" 

            number2="${number#test}"
            #echo "$number2" 

            # Get the answer file name
            answer_file="$answers_dir/ans$number2.txt"

            execute_file="$student_folder/out$number2.txt"


            #echo "Test case file: $test_case_file"
            # Compile the code file
            if [ "$extension" == "c" ]; then
                gcc "$student_folder/main.c" -o "$student_folder/main.out"
            elif [ "$extension" == "java" ]; then
                javac "$student_folder/Main.java" -o "$student_folder/Main.class"
            fi

            # Run the code file and save the output file in the student folder
            
            if [ "$extension" == "c" ]; then
                ./"$student_folder/main.out" < "$test_case_file" > "$execute_file"
            elif [ "$extension" == "java" ]; then
                java -cp "$student_folder/Main.class" < "$test_case_file" > "$execute_file"
            elif [ "$extension" == "python" ]; then
                python3 "$student_folder/main.py" < "$test_case_file" > "$execute_file"
            fi

            # Compare the output file with the answer file
            diff -w "$execute_file" "$answer_file" > /dev/null

            # Get the exit code of the diff command
            exit_code=$?

            # If the exit code is 0, then the output file matches the answer file 
            #keep variable for matched and not matched and increment them
            
            if [ $exit_code -eq 0 ]; then
                #echo "Matched"
                matched=$((matched+1))
            else
                #echo "Not matched"
                not_matched=$((not_matched+1))
            fi

            
        done

        # Append the result to the result CSV file
        echo "$student_id,$extension,$matched,$not_matched" >> "$result_csv"
    done
done




   


    


    

