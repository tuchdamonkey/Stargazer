### StarGazer Reference Links
* [GitHub Repo](https://github.com/tuchdamonkey/Stargazer.git)
* [NexStar Protocol Manual](https://www.nexstarsite.com/DirectConnect.htm)
* [TinyGPS++ Documentation](https://arduiniana.org/libraries/tinygpsplus/)


//========== REFERENCES * GUIDES ================
//--- GitHub ---
* [GitHub Cheat Sheet] (https://education.github.com/git-cheat-sheet-education.pdf)
* [GitHub Visual Guide] (https://ohmygit.org/)







//=========== COMMANDS ===================
//--- GitHub ---
* [git status] tells you what files you've changed and what hasn't been saved yet
* [git add . ] tells Git "i want to include everything in this snapshot
* [git commit -m "..."] permanently saves the current state of code to local history with a note.
* [git push] sends local snapshots to GitHub repository
* [git log --oneline] quicklist of all previous saves
* [git status] status of your files
* [git reflog] shows the history of 'undo' actions
* [git checkout (eg, main, feature-name)] takes you to the snapshot workspace you designate
* [git restore -staged <file>...] use to unstage "git add . "
* [git commit --amend -m "Your new, descriptive message here"] good commit, but wrong message
* [git log --oneline --graph -n 20] use to see snapshot log
    * [git reset --hard a1b2c3d] replace a1b2c3d with the actual commit hash



// -- Branch --
* [git branch] lists all branches, highlights the branch you are currently on.



* [git checkout main] 1. step off the branch you intend to delete
* [git pull origin main] 2. ensure local main matches what is on the remote server

* [git branch -D branch-name] -D deletes the branch name, which you can list via 'git branch'

* [git branch feature-name] creates new branch
* [git checkout feature-name] switches the snapshot workspace to that branch
* [git checkout -b feature-name] -b (branch), (feature-name) create the name of the branch, this command creates then takes you to the branch in command line

* [git push origin <branch name>] this pushes the branch to github

* [git pull --rebase origin feature/ross-soss]  Instead of a "Merge" (which creates a messy extra commit just to join the lines), we will use Rebase. This tells Git: "Take my new local work, set it aside for a second, pull down the changes from GitHub, and then put my work back on top of them."

* [git push -u origin feature/ross-soss] The -u stands for set-upstream. After you run that one time, Git will "remember" the connection. From then on, as long as you are on that branch, you can just type: [git push]

*
*


