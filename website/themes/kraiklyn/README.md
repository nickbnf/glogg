Kraiklyn
=====

A simple one page [Hugo](https://gohugo.io/) theme for documentation. Inspired by Hyde, Simpledoc and docDock themes

# Features
 - all content is rendered on one page
 - unlimited menu level
 - mobile friendly
 - customize website logo
 - add links to the sidebar

# Installation
Clone the repository to your site’s themes directory.

# Usage
All content is rendered on the main page. Front-matter's `anchor` is used for the navigation. Content is ordered by Weight.

When creating a new section, make sure `_index.md` exists for correct cross-references.

## Shortcodes

### block
Create notes, tips and other blocks on the page
```markdown
{{% block note %}}
By default only ports 22, 80 and 443 are open
{{% /block %}}
```

Available types: `note`, `tip`, `warn`, `info`

### anchor
Returns `anchor` parameter (see `archetypes/default.md` file) of the article
```markdown
Check [Proxy environment]({{% anchor "installation/proxy-environment.md" %}}) section
```
Since the theme is a one-page theme, this is the way to make cross-references on the website

## Customizing sidebar

### Changing logo
Replace logo by creating `layouts/partials/logo.html` file

### Adding menu entries to the external links section
Customize the name of the section by adding to the `config.toml`
```toml
[params]
externalTitle = "Surfly docs"
```

Add new entries:
```toml
[[menu.shortcuts]]
name = "Javascript API"
url = "https://docs.surfly.com/javascript.html"
weight = 20
```

### Changing color
Customize the color of the sidebar by adding to the `config.toml`
```toml
[params]
sidebarColor = "green"
```

Available values : default, green, purple, pink, red, cyan, blue, grey, orange.

## Add favicon
Put `favicon.ico` inside `static` folder


## Add custom CSS
You can add your custom CSS files with the `customCss` parameter of the configuration file.

```toml
customCss = ["css/custom.css", "css/custom2.css"]
```

Just put your files in `static/css` directory.
