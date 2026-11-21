<!-- dust:begin -->
## Dust workspace

This project uses the Dust framework. Shared context lives in `.dust/` (project brief, artifacts, lessons).
- Orient with `dust brief`, which shows the summary, git state and open artifacts. Read artifact bodies only as needed.
- Write durable output with `dust artifact new <type> "<title>"` (it creates the right `.dust/artifacts/<type>/` folder on first use; don't create folders by hand). Git follows git-flow (`dust flow status`).
- Don't assume the tech stack. Check `.dust/project.md` or ask.
<!-- dust:end -->
