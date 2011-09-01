$Id

USAGE INSTRUCTIONS:

1. Install the module and configure permissions to allow import.

2. Prepare a site map file 

  Menu structure must follow this example:

    Page1
    - Page2
    - Page3
    Page4
    - Page5
    -- Page6

  or this

  Page1
  * Page2
  ** Page3
  Page4
  * Page5
  ** Page6
  *** Page7

  Also, you may optionally specify path alias (alternative path provided by path module)
  or external URL. Writing it after node's title, separating by semicolon or vertical bar:
  External URLs should ALWAYS start with "http://".

    Page1|page1
    - Page2|page1/page2
    - Page3|page1/page3
    Page4;http://domain.com/
    - Page5;http://mail.com/index.php
    -- Page6;page4/page5/page6

  Space(s) between indentation symbol "*" or "-" and menu/node title are optional,
  however you cannot put spaces between indentation symbols like "* * *" or "-- - -".

  Use examples from menu_samples directory to better understand the syntax and features.

3. Go to Site building -> Menus

4. Select "Menu import" tab

5. Select the site map file created earlier and specify necessary options

6. Submit and see the new menu structure with stub nodes created automatically.

7. Enjoy your saved time ;)

FEATURES:
 - import your site menu(s) using an easy to understand text format
 - import options:
   - option to publish or not publish the stub nodes

