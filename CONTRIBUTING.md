# Contributing to PhASAR

Thank you for your interest in contributing to PhASAR. There are many ways to
contribute, and we appreciate all contributions.

### Bug Reports
If you are working with PhASAR and run into a bug, we most certainly wish to know about it. Please let us know and create a bug report in form of an [issue on Github](https://github.com/secure-software-engineering/phasar/issues/new). Please provide information on your system such as OS (version), compiler (version), etc. If possible, also provide any resource(s) necessary, e.g. LLVM IR files, to reproduce the bug.

### Bug Fixes
If you are interested in contributing code to PhASAR, bugs labeled with the 'bug' and 'good first issue' keywords are a good way to get familiar with the code base. If you are interested in fixing a bug, please assign the bug to yourself such that people know you are working on it.

For each bug that you detect and fix please also provide (a) unit test(s) to ensure that it is gone once and for all.

### Bigger Pieces of Work
In case you are interested in taking on bigger pieces of work, you can also find them labeled as 'extensive' among the other issues. Please recall that we have a [PhASAR Slack channel](https://phasar.slack.com/) in which you can discuss solution candidates and design ideas, and get help from other PhASARists. Drop us an email info@phasar.org to be added to the slack channel.

## How to Submit a Patch
Once you have a patch ready, it is time to submit it. The patch should:

* include (a) small unit test(s)
* conform to our coding standards (which are under development ;-). You can use the `pre-commit` script to automatically format your patch properly.
* not contain any unrelated changes
* be an isolated change. Independent changes should be submitted as separate patches for easier reviews.

To get a patch accepted, create a pull request and await its review by the PhASAR community. To make sure the right people see you patch, please select suitable reviewers and add them to your patch when requesting a review.

A reviewer may request changes or ask questions during the review. If you are unsure about your patch, feel free to ask for guidance on Slack. This cycle continues until all requests and comments have been addressed and a reviewer accepts the patch. Then, the change can be committed.
Please remember that you are asking for valuable time from other professional developers.

## Please Help Us to Improve Phasar
If you are using PhASAR without the wish or expertise to contribute to its code base, you can still help us by providing valuable feedback using this [web from](https://docs.google.com/forms/d/e/1FAIpQLScUXZcdXZe1rY8VxUKjXhTtrsNX5TysNUO4yD8-gaIHiqqWTQ/viewform). Please also refer to [Contributing to PhASAR](https://github.com/secure-software-engineering/phasar/wiki/Contributing-to-PhASAR).

Thanks for contributing to the PhASAR project.

`std::cout << "Thank you!\n";`

## Additional Notes for Student Assistants
When implementing a new feature please name you feature branch according to the following naming scheme:

* `f-CamelCase`
