<?php

/**
 * @file
 * Export functions for menu_import module.
 */

/**
 * Menu export form.
 */
function menu_import_export_form($form, &$form_state) {
  $form['menu_name'] = array(
    '#type' => 'select',
    '#title' => t('Menu to export'),
    '#options' => menu_get_menus(),
    '#required' => TRUE,
    '#default_value' => !empty($form_state['values']['menu_name']) ? $form_state['values']['menu_name'] : NULL,
  );

  $form['line_ending'] = array(
    '#type' => 'select',
    '#title' => t('Line ending'),
    '#options' => array(
      'unix'  => t('Unix / Linux'),
      'mac'   => t('Apple Mac'),
      'dos'  => t('Microsoft DOS'),
    ),
    '#required' => TRUE,
  );

  $form['sumbit'] = array(
    '#type' => 'submit',
    '#value' => t('Export'),
  );

  return $form;
}

/**
 * Menu export handler.
 */
function menu_import_export_form_submit($form, &$form_state) {
  global $me_options;
  $menu_name = $form_state['values']['menu_name'];

  switch ($form_state['values']['line_ending']) {
    case 'unix':
      $me_options['line_ending'] = "\n";
      break;
    case 'mac':
      $me_options['line_ending'] = "\r";
      break;
    case 'dos':
      $me_options['line_ending'] = "\r\n";
      break;
  }

  $tree = menu_build_tree($menu_name);

  // Menu contains items.
  if (count($tree)) {
    $filename = "$menu_name-export.txt";
    $output   = '';
    _menu_import_export_recurse($tree, 0, $output);
    drupal_add_http_header('Content-type', 'text/plain');
    drupal_add_http_header('Content-Disposition', 'attachment; filename="' . $filename . '"');
    drupal_send_headers();
    echo $output;
    exit;
  }
  // Empty menu.
  else {
    drupal_set_message(t('Menu @menu contains no items.', array('@menu' => $menu_name)), 'error');
  }
}

/**
 * Generates export file recursively.
 */
function _menu_import_export_recurse($tree, $level, &$output) {
  global $me_options;

  foreach ($tree as $index => $data) {
    $link = $data['link'];

    $indentation = str_repeat('-', $level);

    $output .=
      $indentation
      . $link['link_title']
      . '|' . $link['link_path']
      . (!empty($link['description']) ? '|' . $link['description'] : '')
      . $me_options['line_ending'];

    if ($data['below']) {
      _menu_import_export_recurse($data['below'], $level + 1, $output);
    }
  }
}
