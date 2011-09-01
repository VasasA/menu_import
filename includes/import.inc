<?php

/**
 * @file
 * Import functions for menu_import module.
 */

/**
 * Helper function to find node associated to path.
 */
function _menu_import_lookup_path($path, $title, array $options) {
  echo $title;
  $nid = FALSE;
  $is_node = FALSE;

  if ($options['search_alias']) {
    $system_url = drupal_lookup_path('source', $path, $options['node_language']);
    if (!$system_url) {
      // No such alias in the system.
      $system_url = $path;
    }
  }
  else {
    $system_url = $path;
  }

  if (drupal_substr($system_url, 0, 4) == 'node') {
    $is_node = TRUE;
    $nid = drupal_substr($system_url, 5);
    if (!is_numeric($nid)) {
      $nid = FALSE;
    }
  }

  if (!$nid && $options['search_title']) {
    // Search node by title.
    $nid = db_select('node', 'n')
      ->fields('n', array('nid'))
      ->condition('n.title', $title)
      ->condition('n.language', $options['node_language'])
      ->execute()->fetchField();
  }

  $result['nid'] = FALSE;
  if ($nid || $is_node) {
    $system_url = 'node/' . $nid;
    if ($options['node_attach'] == MI_NODE_LINK && !drupal_valid_path($system_url)) {
      $result['link_path'] = '';
    }
    else {
      $result['link_path'] = $system_url;
    }
    $result['nid'] = $nid;
  }
  elseif (!drupal_valid_path($system_url)) {
    $result['link_path'] = '';
  }
  else {
    $result['link_path'] = "$system_url";
  }

  return $result;
}

/**
 * Parse a line of text containing the menu structure.
 */
function menu_import_parse_line($line, $prev_level, array $weights, array $parents, array $options) {
  $menuitem = array(
    'error' => FALSE,
    'link_title' => NULL,
    'children' => array(),
    'parent' => NULL,
    'nid' => FALSE,
    'path' => FALSE,
    'weight' => 0,
    'external' => FALSE,
    'level' => 0,
  );

  $line = trim($line);
  // Only * and - are allowed as indentation characters
  // URL aliases are separated from title by vertical bar or semi-colon
  preg_match('/^([\-]+|[\*]+)?(\s+)?([^\|;]*)([\|;]{1})?([^\|;]*)?/', $line, $matches);

  $level = drupal_strlen($matches[1]);
  $title = strip_tags($matches[3]);
  $path  = trim($matches[5]);

  // Skip empty items
  if (!strlen($title)) {
    $menuitem['error'] = t('missing menu title');
    return $menuitem;
  }

  // Make sure this item is only 1 level below the last item.
  if ($level > $prev_level + 1) {
    $menuitem['error'] = t('wrong indentation');
    return $menuitem;
  }

  if (isset($weights[$level])) {
    if ($level > $prev_level) {
      $weight = 0;
    }
    else {
      $weight = $weights[$level] + 1;
    }
  }
  else {
    $weight = 0;
  }
  $menuitem['weight'] = $weight;
  $menuitem['parent'] = !$level ? 0 : $parents[$level - 1];
  $menuitem['link_title'] = $title;
  $menuitem['level'] = $level;
  $menuitem['path'] = $path;

  if (url_is_external($path)) {
    $menuitem['external'] = TRUE;
    $menuitem['link_path'] = $path;
  }
  else {
    $result = _menu_import_lookup_path($path, $title, $options);
    $menuitem['link_path'] = $result['link_path'];
    $menuitem['nid'] = $result['nid'];
  }

  return $menuitem;
}

/**
 * File parser function. Reads through the text file and constructs the menu.
 *
 * @param $uri
 *   uri of the uploaded file
 * @param $menu_name
 *   internal name of existiong menu
 * @param $options
 *   An associative array of search options.
 *   - search_alias: search node by alias
 *   - search_title: search node by title
 *   - node_language: language to use
 *
 * @return array
 *   array structure of menu
 */
function menu_import_parse_menu_from_file($uri, $menu_name, array $options) {
  $menu = array(
    'errors' => array(),
    0 => array(
      'menu_name' => $menu_name,
      'children' => array(),
    )
  );

  // Keep track of actual weights per level.
  $weights = array();
  // Keep track of actual parents per level.
  $parents = array();

  $handle = fopen($uri, "r");
  if (!$handle) {
    $menu['errors'][] = t("Couldn't open the uploaded file for reading.");
    return $menu;
  }

  $level = $current_line = 0;
  while ($line = fgets($handle)) {
    $current_line++;

    $menuitem = menu_import_parse_line($line, $level, $weights, $parents, $options);
    if ($menuitem['error']) {
      $menu['errors'][] = t('Error on line @line_number: @error.', array('@line_number' => $current_line, '@error' => $menuitem['error']));
    }
    $menu[$current_line] = $menuitem;
    $menu[$menuitem['parent']]['children'][] = $current_line;

    $level = $menuitem['level'];
    $parents[$level] = $current_line;
    $weights[$level] = $menuitem['weight'];
  }

  fclose($handle);

  return $menu;
}

/**
 * Text parser function. Reads through the text and constructs the menu.
 *
 * @param $text
 *   text containing the menu structure
 *
 * @see menu_import_parse_menu_from_file().
 */
function menu_import_parse_menu_from_string($text, $menu_name, array $options) {
  $menu = array(
    'errors' => array(),
    0 => array(
      'menu_name' => $menu_name,
      'children' => array(),
    )
  );

  // Keep track of actual weights per level.
  $weights = array();
  // Keep track of actual parents per level.
  $parents = array();

  $level = $current_line = 0;
  $lines = explode("\n", $text);
  foreach ($lines as $line) {
    $current_line++;

    $menuitem = menu_import_parse_line($line, $level, $weights, $parents, $options);
    if ($menuitem['error']) {
      $menu['errors'][] = t('Error on line @line_number: @error.', array('@line_number' => $current_line, '@error' => $menuitem['error']));
    }
    $menu[$current_line] = $menuitem;
    $menu[$menuitem['parent']]['children'][] = $current_line;

    $level = $menuitem['level'];
    $parents[$level] = $current_line;
    $weights[$level] = $menuitem['weight'];
  }

  return $menu;
}

/**
 * Import menu items.
 *
 * @param $menu
 *   An associative array containing the menu structure.
 * @param $options
 *   An associative array of import options.
 *   - node_attach: how to attach the node (see MI_NODE_* constants)
 *   - node_type: node type
 *   - node_language: node language
 *   - node_status: node status
 */
function menu_import_save_menu($menu, $options) {
  $nodes_deleted_cnt = $unknown_links_cnt = $external_links_cnt = 0;
  $nodes_matched_cnt = $nodes_new_cnt = $failed_cnt = 0;

  $menu_name = $menu[0]['menu_name'];
  if ($options['node_attach'] == MI_NODE_RECREATE) {
    $nodes_deleted_cnt = menu_import_delete_menuitems_and_nodes($menu_name);
  }

  $menu[0]['mlid'] = 0;

  foreach ($menu as $item) {
    if (!isset($item['children'])) {
      continue;
    }
    foreach ($item['children'] as $index) {
      $menuitem = $menu[$index];
      $menuitem['plid'] = $menu[$menuitem['parent']]['mlid'];
      $menuitem['menu_name'] = $menu_name;
      $menuitem['description'] = '';

      if ($menuitem['external']) {
        $external_links_cnt++;
      }
      elseif (isset($menuitem['link_path']) && substr($menuitem['link_path'], 1, 4) != 'node') {
        // This is not a node.
        $unknown_links_cnt++;
      }
      else {
        // Attach node.
        $nid = menu_import_attach_node($menuitem['nid'], $menuitem['link_title'], $options);
        if ($nid === $menuitem['nid']) {
          menu_import_delete_menuitem($menuitem);
          $nodes_matched_cnt++;
          $menuitem['link_path'] = 'node/' . $nid;
        }
        elseif ($nid) {
          $nodes_new_cnt++;
          $menuitem['link_path'] = 'node/' . $nid;
        }
        else {
          $unknown_links_cnt++;
          if (empty($menuitem['link_path'])) {
            $menuitem['link_path'] = '<front>';
          }
        }
      }

      // Save menuitem and set mlid.
      $mlid = menu_link_save($menuitem);
      if (!$mlid) {
        $failed_cnt++;
      }
      $menu[$index]['mlid'] = $mlid;
    }
  }

  return array(
    'external_links' => $external_links_cnt,
    'unknown_links' => $unknown_links_cnt,
    'matched_nodes' => $nodes_matched_cnt,
    'new_nodes' => $nodes_new_cnt,
    'deleted_nodes' => $nodes_deleted_cnt,
    'failed' => $failed_cnt,
  );
}

/**
 * Attach node.
 *
 * @param $nid
 *   node nid
 * @param $title
 *   node title
 * @param $options
 *   - node_attach: how to attach the node
 *   - node_type: node type
 *   - node_language: node language
 *   - node_status: node_status
 */
function menu_import_attach_node($nid = 0, $title, $options) {
  switch ($options['node_attach']) {
    case MI_NODE_LINK:
      if ($nid) {
        return $nid;
      }
      return 0;
      break;

    case MI_NODE_LINK_CREATE:
      if ($nid) {
        return $nid;
      }
      break;

    case MI_NODE_RECREATE:
      break;
  }

  $node = new stdClass();
  $node->title = $title;
  $node->uid = empty($GLOBALS['user']->uid) ? 1 : $GLOBALS['user']->uid;
  $node->type = $options['node_type'];
  $node->body = '';
  $node->status = $options['node_status'];
  $node->language = $options['node_language'];
  node_save($node);
  return $node->nid;
}

/**
 * Delete nodes attached to menu.
 */
function menu_import_delete_menuitems_and_nodes($menu_name) {
  $menuitems = db_select('menu_links', 'ml')
    ->fields('ml', array('mlid', 'link_path'))
    ->condition('ml.menu_name', $menu_name)
    ->execute()
    ->fetchAllAssoc('mlid');

  $deleted_cnt = 0;
  foreach ($menuitems as $menuitem) {
    $link = explode('/', $menuitem['link_path']);
    // Delete nodes only
    if ($link[0] == 'node' && is_numeric($link[1])) {
      $nid = $link[1];
      db_delete('node')->condition('nid', $nid)->execute();
      db_delete('node_revisions')->condition('nid', $nid)->execute();
      db_delete('menu_links')->condition('mlid', $menuitem['mlid'])->execute();
      $deleted_cnt++;
    }
  }
  return $deleted_cnt;
}

/**
 * Delete menu item by nid.
 */
function menu_import_delete_menuitem($menuitem) {
  db_delete('menu_links')
    ->condition('menu_name', $menuitem['menu_name'])
    ->condition('link_path', 'node/' . $menuitem['nid'])
    ->execute();
}
