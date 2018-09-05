# HOWTO Release navitia

You should first have a proper python env:
```
pip install -r requirements_release.txt -U
```

## "Normal" release (minor)

First have a look on github's repo at PRs about to be released https://github.com/CanalTP/navitia/pulls?q=is%3Apr+is%3Aclosed+sort%3Aupdated-desc
* Check that `not_in_changelog` and `hotfix` labels are correct and none is missing on PRs that are gonna be released
* Check that titles are correct (clear, with the component impacted)

Then the script should take over:
```
cd <path/to/repo/navitia>
# to be sure to launch the correct version of the release script
git fetch <canaltp_distant_repo_name> && git rebase <canaltp_distant_repo_name>/dev dev
python ./script_release.py minor <canaltp_distant_repo_name>
```
Then follow the instructions given by the script, and also:
* pay attention to the changelog, remove useless PR (small doc) and check that every important PR is there
* don't forget to make `git submodule update --recursive` when necessary
* check that `release` branch COMPILES and TESTS (unit, docker and tyr) before pushing it!

## Other releases

### For a major release, same as minor, but major:
```
python ./script_release.py major <canaltp_distant_repo_name>
```

### For hotfix:
```
python ./script_release.py hotfix <canaltp_distant_repo_name>
```
A new branch has been created <release_x.yy.z> and the changelog is opened.
Then the process is less automated (but still, instructions are given):
* Populate the changelog :
	Add the hotfix PR name and link to github (as for the automated changelog in the release process)
* Cherry-pick the commits you want to release:
	```
	git cherry-pick <commit_id> # Each commit_id of the hotfix PR
	```
* Merge the content of the new release branch with the hotfix commits to the 'release' branch:
	```
	git checkout release		
	git merge --no-ff <release_x.yy.z>
	```
* Tag the new release:
	``` 
	git tag -a vx.yy.z
	```
    _Minor_: You will have to populate the tag with correct blank lines if you want a nice github changelog:
    ```
    Version 2.57.0

        * Kraken: Add ptref shortcut between physical_mode and jpps  <https://github.com/CanalTP/navitia/pull/2417>
    ```
* Push the release branch to the dev branch
	``` 
	git push upstream release dev --tags
	```

# Troubleshooting
If you run into github's daily limitation, you can easily provide your login/token into the script.
Search for "rate limit exceeded" in script.
