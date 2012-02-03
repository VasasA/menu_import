<?php

/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
function _menu_import_export_recurse($tree, $level, &$output) {
  foreach ($tree as $index => $data) {
    $link = $data['link'];

    $indentation = str_repeat('-', $level);

    $output .=
      $indentation
      . $link['link_title']
      . '|' . $link['link_path']
      . (!empty($link['description']) ? '|' . $link['description'] : '')
      . "\n";

    if ($data['below']) {
      _menu_import_export_recurse($data['below'], $level + 1, $output);
    }
  }
}
