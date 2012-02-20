<?php

/**
 * @file
 *   Menu import module drush integration.
 */

/**
 * Implementation of hook_drush_command().
 *
 * @See drush_parse_command() for a list of recognized keys.
 *
 * @return
 *   An associative array describing your command(s).
 */
function menu_import_drush_command() {
  $items = array();

  $items['menu-import'] = array(
    'description' => 'Imports a menu from a text file.',
    'callback' => 'drush_menu_import_import',
    'drupal dependencies' => array('menu_import'),
    'examples' => array(
      'drush menu-import main_menu menu_file.txt' => 'Import menu_file.txt into menu called main_menu using no specific options.',
      'drush menu-import main_menu menu_file.txt --create-content --link-content --clean-import'
        => 'Import menu_file.txt into menu called main_menu, creates empty content, tries to link to existsing and deletes all menu items before importing new ones.',
    ),
    'arguments' => array(
      'file' => 'Required. Path to the menu text file.',
      'menu-name' => 'Required. Menu name to import in.',
    ),
    'options' => array(
      'create-content'=> 'Creates empty nodes referenced by respective menu items. Useful for stubbing-out a new site.',
      'link-content'  => 'Tries to link menu items to existing content/pages.',
      'clean-import'  => 'Removes old menu items. Use carefully!',
    ),
    'aliases' => array('mi-import', 'mi'),
  );

  $items['menu-export'] = array(
    'description' => 'Exports menu to a text file.',
    'callback' => 'drush_menu_import_export',
    'drupal dependencies' => array('menu_import'),
    'examples' => array(
      'drush menu-export main_menu menu_file.txt' => 'Export menu called main_menu to a text file menu_file.txt.',
    ),
    'arguments' => array(
      'file' => 'Required. Path to the resulting menu text file.',
      'menu-name' => 'Required. Menu name to export.',
    ),
    'aliases' => array('mi-export', 'me'),
  );

  return $items;
}

/**
 * Implementation of hook_drush_help().
 */
function menu_import_drush_help($section) {
  switch ($section) {
    case 'drush:menu-import':
      return dt('Imports a menu from a text file.');
    case 'drush:menu-export':
      return dt('Exports menu to a text file.');
  }
}
