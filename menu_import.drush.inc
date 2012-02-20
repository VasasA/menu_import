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

/**
 * Get a list of all feature modules.
 */
function drush_features_list() {
  module_load_include('inc', 'features', 'features.export');
  $rows = array(array(dt('Name'), dt('Feature'), dt('Status'), dt('State')));
  foreach (features_get_features(NULL, TRUE) as $k => $m) {
    switch (features_get_storage($m->name)) {
      case FEATURES_DEFAULT:
      case FEATURES_REBUILDABLE:
        $storage = '';
        break;
      case FEATURES_OVERRIDDEN:
        $storage = dt('Overridden');
        break;
      case FEATURES_NEEDS_REVIEW:
        $storage = dt('Needs review');
        break;
    }
    $rows[] = array(
      $m->info['name'],
      $m->name,
      $m->status ? dt('Enabled') : dt('Disabled'),
      $storage
    );
  }
  drush_print_table($rows, TRUE);
}

/**
 * Create a feature module based on a list of components.
 */
function drush_features_export() {
  $args = func_get_args();

  if (count($args) == 1) {
    // Assume that the user intends to create a module with the same name as the
    // "value" of the component.
    list($source, $component) = explode(':', $args[0]);
    $stub = array($source => array($component));
    $sources = features_get_components();
    if (empty($component) || empty($sources[$source])) {
      _features_drush_set_error($args[0], 'FEATURES_COMPONENT_NOT_FOUND');
      exit;
    }
    else {
      _drush_features_export($stub, $component);
    }
  }
  elseif (count($args) > 1) {
    // Assume that the user intends to create a new module based on a list of
    // components. First argument is assumed to be the name.
    $name = array_shift($args);
    $stub = array();
    foreach ($args as $v) {
      list($source, $component) = explode(':', $v);
      $stub[$source][] = $component;
    }
    _drush_features_export($stub, array(), $name);
  }
  else {
    $rows = array(array(dt('Available sources')));
    foreach (features_get_components(TRUE) as $component => $info) {
      if ($options = features_invoke($component, 'features_export_options')) {
        foreach ($options as $key => $value) {
          $rows[] = array($component .':'. $key);
        }
      }
    }
    drush_print_table($rows, TRUE);
  }
}

/**
 * Update an existing feature module.
 */
function drush_features_update() {
  if ($args = func_get_args()) {
    foreach ($args as $module) {
      if (($feature = feature_load($module, TRUE)) && module_exists($module)) {
        _drush_features_export($feature->info['features'], $feature->info['dependencies'], $feature->name, dirname($feature->filename));
      }
      else if ($feature) {
        _features_drush_set_error($module, 'FEATURES_FEATURE_NOT_ENABLED');
      }
      else {
        _features_drush_set_error($module);
      }
    }
  }
  else {
    // By default just show contexts that are available.
    $rows = array(array(dt('Available features')));
    foreach (features_get_features(NULL, TRUE) as $name => $info) {
      $rows[] = array($name);
    }
    drush_print_table($rows, TRUE);
  }
}

/**
 * Update all enabled features. Optionally pass in a list of features to
 * exclude from being updated.
 */
function drush_features_update_all() {
  $features_to_update = array();
  $features_to_exclude = func_get_args();
  foreach (features_get_features() as $module) {
    if ($module->status && !in_array($module->name, $features_to_exclude)) {
      $features_to_update[] = $module->name;
    }
  }
  drush_print(dt('The following modules will be updated: !modules', array('!modules' => implode(', ', $features_to_update))));
  if (drush_confirm(dt('Do you really want to continue?'))) {
    foreach ($features_to_update as $module_name) {
      drush_backend_invoke('features-update '. $module_name);
    }
  }
  else {
    drush_die('Aborting.');
  }
}

/**
 * Write a module to the site dir.
 *
 * @param $requests
 *   An array of the context requested in this export.
 * @param $module_name
 *  Optional. The name for the exported module.
 */
function _drush_features_export($stub, $dependencies, $module_name = NULL, $directory = NULL) {
  $root = drush_get_option(array('r', 'root'), drush_locate_root());
  if ($root) {
    $directory = isset($directory) ? $directory : 'sites/all/modules/' . $module_name;
    if (is_dir($directory)) {
      drush_print(dt('Module appears to already exist in !dir', array('!dir' => $directory)));
      if (!drush_confirm(dt('Do you really want to continue?'))) {
        drush_die('Aborting.');
      }
    }
    else {
      drush_op('mkdir', $directory);
    }
    if (is_dir($directory)) {
      drupal_flush_all_caches();
      module_load_include('inc', 'features', 'features.export');
      $export = features_populate($stub, $dependencies, $module_name);
      if (!feature_load($module_name)) {
        $export['name'] = $module_name;
      }
      $files = features_export_render($export, $module_name, TRUE);
      foreach ($files as $extension => $file_contents) {
        if (!in_array($extension, array('module', 'info'))) {
          $extension .= '.inc';
        }
        drush_op('file_put_contents', "{$directory}/{$module_name}.$extension", $file_contents);
      }
      drush_log(dt("Created module: !module in !directory", array('!module' => $module_name, '!directory' => $directory)), 'ok');
    }
    else {
      drush_die(dt('Couldn\'t create directory !directory', array('!directory' => $directory)));
    }
  }
  else {
    drush_die(dt('Couldn\'t locate site root'));
  }
}

/**
 * Revert a feature to it's code definition.
 */
function drush_features_revert() {
  if ($args = func_get_args()) {
    module_load_include('inc', 'features', 'features.export');
    features_include();

    // Determine if revert should be forced.
    $force = drush_get_option('force');
    foreach ($args as $module) {
      if (($feature = feature_load($module, TRUE)) && module_exists($module)) {

        $components = array();
        // Forcefully revert all components of a feature.
        if ($force) {
          foreach (array_keys($feature->info['features']) as $component) {
            if (features_hook($component, 'features_revert')) {
              $components[] = $component;
            }
          }
        }
        // Only revert components that are detected to be Overridden/Needs review.
        else {
          $states = features_get_component_states(array($feature->name), FALSE);
          foreach ($states[$feature->name] as $component => $state) {
            if (in_array($state, array(FEATURES_OVERRIDDEN, FEATURES_NEEDS_REVIEW)) && features_hook($component, 'features_revert')) {
              $components[] = $component;
            }
          }
        }

        if (empty($components)) {
          drush_log(dt('Current state already matches defaults, aborting.'), 'ok');
        }
        else {
          foreach ($components as $component) {
            if (drush_confirm(dt('Do you really want to revert !component?', array('!component' => $component)))) {
              features_revert(array($module => array($component)));
              drush_log(dt('Reverted !component.', array('!component' => $component)), 'ok');
            }
            else {
              drush_log(dt('Skipping !component.', array('!component' => $component)), 'ok');
            }
          }
        }
      }
      else if ($feature) {
        _features_drush_set_error($module, 'FEATURES_FEATURE_NOT_ENABLED');
      }
      else {
        _features_drush_set_error($module);
      }
    }
  }
  else {
    drush_features_list();
    return;
  }
}

/**
 * Revert all enabled features to their definitions in code. Optionally pass in
 * a list of features to exclude from being reverted.
 */
function drush_features_revert_all() {
  $features_to_revert = array();
  $features_to_exclude = func_get_args();
  foreach (features_get_features() as $module) {
    if ($module->status && !in_array($module->name, $features_to_exclude)) {
      $features_to_revert[] = $module->name;
    }
  }
  drush_print(dt('The following modules will be reverted: !modules', array('!modules' => implode(', ', $features_to_revert))));
  if (drush_confirm(dt('Do you really want to continue?'))) {
    foreach ($features_to_revert as $module_name) {
      drush_backend_invoke('features-revert '. $module_name);
    }
  }
  else {
    drush_die('Aborting.');
  }
}

/**
 * Show the diff of a feature module.
 */
function drush_features_diff() {
  if (!$args = func_get_args()) {
    drush_features_list();
    return;
  }
  $module = $args[0];
  $feature = feature_load($module);
  if (!module_exists($module)) {
    drush_log(dt('No such feature is enabled: ' . $module), 'error');
    return;
  }
  module_load_include('inc', 'features', 'features.export');
  $overrides = features_detect_overrides($feature);
  if (empty($overrides)) {
    drush_log(dt('Feature is in its default state. No diff needed.'), 'ok');
    return;
  }
  module_load_include('inc', 'diff', 'diff.engine');

  if (!class_exists('DiffFormatter')) {
    if (drush_confirm(dt('It seems that the Diff module is not available. Would you like to download and enable it?'))) {
      // Download it if it's not already here.
      $project_info = drush_get_projects();
      if (empty($project_info['diff']) && !drush_backend_invoke('dl diff')) {
        return drush_set_error(dt('Diff module could not be downloaded.'));
      }

      if (!drush_backend_invoke('en diff')) {
        return drush_set_error(dt('Diff module could not be enabled.'));
      }
    }
    else {
      return drush_set_error(dt('Diff module is not enabled.'));
    }
    // If we're still here, now we can include the diff.engine again.
    module_load_include('inc', 'diff', 'diff.engine');
  }

  $formatter = new DiffFormatter();
  $formatter->leading_context_lines = 2;
  $formatter->trailing_context_lines = 2;
  $formatter->show_header = FALSE;

  foreach ($overrides as $component => $items) {
    $diff = new Diff(explode("\n", $items['default']), explode("\n", $items['normal']));
    drush_print();
    drush_print(dt("Component: !component", array('!component' => $component)));
    $rows = explode("\n", $formatter->format($diff));
    if (drush_get_context('DRUSH_NOCOLOR')) {
      $red = $green = "%s";
    }
    else {
      $red = "\033[31;40m\033[1m%s\033[0m";
      $green = "\033[0;32;40m\033[1m%s\033[0m";
    }
    foreach ($rows as $row) {
      if (strpos($row, '>') === 0) {
        drush_print(sprintf($green, $row));
      }
      elseif (strpos($row, '<') === 0) {
        drush_print(sprintf($red, $row));
      }
      else {
        drush_print($row);
      }
    }
  }
}

/**
 * Helper function to call drush_set_error().
 *
 * @param $feature
 *   The string name of the feature.
 * @param $error
 *   A text string identifying the type of error.
 * @return
 *   FALSE.  See drush_set_error().
 */
function _features_drush_set_error($feature, $error = '') {
  $args = array('!feature' => $feature);

  switch ($error) {
    case 'FEATURES_FEATURE_NOT_ENABLED':
      $message = 'The feature !feature is not enabled.';
      break;
    case 'FEATURES_COMPONENT_NOT_FOUND':
      $message = 'The given component !feature could not be found.';
      break;
    default:
      $error = 'FEATURES_FEATURE_NOT_FOUND';
      $message = 'The feature !feature could not be found.';
  }

  return drush_set_error($error, dt($message, $args));
}
