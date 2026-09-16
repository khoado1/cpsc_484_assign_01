• Name of student: Khoa Do
• Operating System of your Environment: macos
• Did you collaborate with anyone in the class? If so, let us know who you talked to and what sort of help
you gave or received.: No
• Were there any references (books, papers, websites, etc.) that you found particularly helpful for
completing your assignment? Please provide a list.: No
• Are there any known problems with your code? If so, please provide a list and, if possible, describe what
you think the cause is and how you might fix them if you had more time or motivation. This is very
important, as we're much more likely to assign partial credit if you help us understand what's going on.: No
• Got any comments about this assignment that you'd like to share? Was it too long? Too hard? Were the
requirements unclear? Did you have fun, or did you hate it? Did you learn something, or was it a total
waste of your time? Feel free to be brutally honest; I promise I won't take it personally.: No







clang++ main.cpp glad.c -o assignment_0 -I. \
-I/opt/homebrew/include -L/opt/homebrew/lib \
-lglfw -framework OpenGL -framework Cocoa \
-framework IOKit -framework CoreVideo \
-DGL_SILENCE_DEPRECATION

#setup

Ubuntu
sudo apt update
sudo apt install build-essential g++ gcc libgl-dev libglfw3 libglfw3-dev libglm-dev mesa-utils make cmake git bzip2 tar

macos
brew install glfw cmake
brew install glm

#includes
/opt/homebrew/include

./assignment_1

//outside of submission folder
tar -czvf "CPSC 484 - Assignment 0 - Khoa Do.tar.gz" submission/

Assignment submission: https://csufullerton.instructure.com/courses/3625063/assignments/40215275?module_item_id=93137612

#clone repository
git clone -b main https://github.com/khoado1/cpsc_484_assign_01.git

# Navigate to your project folder
cd /path/to/your/local/project

# Initialize a local Git repository
git init

# Add all project files to the staging area
git add .

# Commit the files with an initial message
git commit -m "Initial commit"

# Rename your default branch to 'main' (standard for GitHub)
git branch -M main

# Link the remote repository as 'origin'
git remote add origin <PASTE_YOUR_GITHUB_URL_HERE>
branch: https://github.com/khoado1/cpsc_484_assign_01.git
git remote add origin https://github.com/khoado1/cpsc_484_assign_01.git

# Verify the remote URL is mapped correctly
git remote -v

git add .
git commit -m "Your commit message here"
git push -u origin main

git add .; git commit -m "checkpoint commit"; git push -u origin main


clear; make clean; make all