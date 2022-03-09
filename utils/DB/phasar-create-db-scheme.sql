/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

-- MySQL Workbench Synchronization
-- Generated: 2017-12-22 10:34
-- Model: New Model
-- Version: 1.0
-- Project: Name of the project
-- Author: Philipp Dominik Schubert

SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0;
SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0;
SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='TRADITIONAL,ALLOW_INVALID_DATES';

CREATE SCHEMA IF NOT EXISTS `phasardb` DEFAULT CHARACTER SET utf8 ;

CREATE TABLE IF NOT EXISTS `phasardb`.`function` (
  `function_id` INT(11) NOT NULL AUTO_INCREMENT,
  `identifier` VARCHAR(512) NULL DEFAULT NULL,
  `declaration` TINYINT(1) NULL DEFAULT NULL,
  `hash` VARCHAR(512) NULL DEFAULT NULL,
  PRIMARY KEY (`function_id`))
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8
COLLATE = utf8_unicode_ci;

CREATE TABLE IF NOT EXISTS `phasardb`.`ifds_ide_summary` (
  `ifds_ide_summary_id` INT(11) NOT NULL AUTO_INCREMENT,
  `analysis` VARCHAR(512) NULL DEFAULT NULL,
  `representation` BLOB NULL DEFAULT NULL,
  PRIMARY KEY (`ifds_ide_summary_id`))
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8
COLLATE = utf8_unicode_ci;

CREATE TABLE IF NOT EXISTS `phasardb`.`module` (
  `module_id` INT(11) NOT NULL AUTO_INCREMENT,
  `identifier` VARCHAR(512) NULL DEFAULT NULL,
  `hash` VARCHAR(512) NULL DEFAULT NULL,
  `code` BLOB NULL DEFAULT NULL,
  PRIMARY KEY (`module_id`))
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8
COLLATE = utf8_unicode_ci;

CREATE TABLE IF NOT EXISTS `phasardb`.`type` (
  `type_id` INT(11) NOT NULL AUTO_INCREMENT,
  `identifier` VARCHAR(512) NULL DEFAULT NULL,
  PRIMARY KEY (`type_id`))
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8
COLLATE = utf8_unicode_ci;

CREATE TABLE IF NOT EXISTS `phasardb`.`type_hierarchy` (
  `type_hierarchy_id` INT(11) NOT NULL AUTO_INCREMENT,
  `representation` BLOB NULL DEFAULT NULL,
  `representation_ref` VARCHAR(512) NULL DEFAULT NULL,
  PRIMARY KEY (`type_hierarchy_id`))
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8
COLLATE = utf8_unicode_ci;

CREATE TABLE IF NOT EXISTS `phasardb`.`global_variable` (
  `global_variable_id` INT(11) NOT NULL AUTO_INCREMENT,
  `identifier` VARCHAR(512) NULL DEFAULT NULL,
  `declaration` TINYINT(1) NULL DEFAULT NULL,
  PRIMARY KEY (`global_variable_id`))
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8
COLLATE = utf8_unicode_ci;

CREATE TABLE IF NOT EXISTS `phasardb`.`callgraph` (
  `callgraph_id` INT(11) NOT NULL AUTO_INCREMENT,
  `representation` BLOB NULL DEFAULT NULL,
  `representation_ref` VARCHAR(512) NULL DEFAULT NULL,
  PRIMARY KEY (`callgraph_id`))
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8
COLLATE = utf8_unicode_ci;

CREATE TABLE IF NOT EXISTS `phasardb`.`points-to_graph` (
  `points-to_graph_id` INT(11) NOT NULL AUTO_INCREMENT,
  `representation` BLOB NULL DEFAULT NULL,
  `representation_ref` VARCHAR(512) NULL DEFAULT NULL,
  PRIMARY KEY (`points-to_graph_id`))
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8
COLLATE = utf8_unicode_ci;

CREATE TABLE IF NOT EXISTS `phasardb`.`project` (
  `project_id` INT(11) NOT NULL AUTO_INCREMENT,
  `identifier` VARCHAR(512) NULL DEFAULT NULL,
  PRIMARY KEY (`project_id`))
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8
COLLATE = utf8_unicode_ci;

CREATE TABLE IF NOT EXISTS `phasardb`.`project_has_module` (
  `project_id` INT(11) NOT NULL,
  `module_id` INT(11) NOT NULL,
  PRIMARY KEY (`project_id`, `module_id`),
  INDEX `fk_project_has_module_module1_idx` (`module_id` ASC),
  INDEX `fk_project_has_module_project1_idx` (`project_id` ASC),
  CONSTRAINT `fk_project_has_module_project1`
    FOREIGN KEY (`project_id`)
    REFERENCES `phasardb`.`project` (`project_id`)
    ON DELETE CASCADE
    ON UPDATE CASCADE,
  CONSTRAINT `fk_project_has_module_module1`
    FOREIGN KEY (`module_id`)
    REFERENCES `phasardb`.`module` (`module_id`)
    ON DELETE CASCADE
    ON UPDATE CASCADE)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8
COLLATE = utf8_unicode_ci;

CREATE TABLE IF NOT EXISTS `phasardb`.`module_has_callgraph` (
  `module_id` INT(11) NOT NULL,
  `callgraph_id` INT(11) NOT NULL,
  PRIMARY KEY (`module_id`, `callgraph_id`),
  INDEX `fk_module_has_callgraph_callgraph1_idx` (`callgraph_id` ASC),
  INDEX `fk_module_has_callgraph_module1_idx` (`module_id` ASC),
  CONSTRAINT `fk_module_has_callgraph_module1`
    FOREIGN KEY (`module_id`)
    REFERENCES `phasardb`.`module` (`module_id`)
    ON DELETE CASCADE
    ON UPDATE CASCADE,
  CONSTRAINT `fk_module_has_callgraph_callgraph1`
    FOREIGN KEY (`callgraph_id`)
    REFERENCES `phasardb`.`callgraph` (`callgraph_id`)
    ON DELETE CASCADE
    ON UPDATE CASCADE)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8
COLLATE = utf8_unicode_ci;

CREATE TABLE IF NOT EXISTS `phasardb`.`callgraph_has_points-to_graph` (
  `callgraph_id` INT(11) NOT NULL,
  `points-to_graph_id` INT(11) NOT NULL,
  PRIMARY KEY (`callgraph_id`, `points-to_graph_id`),
  INDEX `fk_callgraph_has_points-to_graph_points-to_graph1_idx` (`points-to_graph_id` ASC),
  INDEX `fk_callgraph_has_points-to_graph_callgraph1_idx` (`callgraph_id` ASC),
  CONSTRAINT `fk_callgraph_has_points-to_graph_callgraph1`
    FOREIGN KEY (`callgraph_id`)
    REFERENCES `phasardb`.`callgraph` (`callgraph_id`)
    ON DELETE CASCADE
    ON UPDATE CASCADE,
  CONSTRAINT `fk_callgraph_has_points-to_graph_points-to_graph1`
    FOREIGN KEY (`points-to_graph_id`)
    REFERENCES `phasardb`.`points-to_graph` (`points-to_graph_id`)
    ON DELETE CASCADE
    ON UPDATE CASCADE)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8
COLLATE = utf8_unicode_ci;

CREATE TABLE IF NOT EXISTS `phasardb`.`module_has_function` (
  `module_id` INT(11) NOT NULL,
  `function_id` INT(11) NOT NULL,
  PRIMARY KEY (`module_id`, `function_id`),
  INDEX `fk_module_has_function_function1_idx` (`function_id` ASC),
  INDEX `fk_module_has_function_module1_idx` (`module_id` ASC),
  CONSTRAINT `fk_module_has_function_module1`
    FOREIGN KEY (`module_id`)
    REFERENCES `phasardb`.`module` (`module_id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  CONSTRAINT `fk_module_has_function_function1`
    FOREIGN KEY (`function_id`)
    REFERENCES `phasardb`.`function` (`function_id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8
COLLATE = utf8_unicode_ci;

CREATE TABLE IF NOT EXISTS `phasardb`.`module_has_global_variable` (
  `module_id` INT(11) NOT NULL,
  `global_variable_id` INT(11) NOT NULL,
  PRIMARY KEY (`module_id`, `global_variable_id`),
  INDEX `fk_module_has_global_variable_global_variable1_idx` (`global_variable_id` ASC),
  INDEX `fk_module_has_global_variable_module1_idx` (`module_id` ASC),
  CONSTRAINT `fk_module_has_global_variable_module1`
    FOREIGN KEY (`module_id`)
    REFERENCES `phasardb`.`module` (`module_id`)
    ON DELETE CASCADE
    ON UPDATE CASCADE,
  CONSTRAINT `fk_module_has_global_variable_global_variable1`
    FOREIGN KEY (`global_variable_id`)
    REFERENCES `phasardb`.`global_variable` (`global_variable_id`)
    ON DELETE CASCADE
    ON UPDATE CASCADE)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8
COLLATE = utf8_unicode_ci;

CREATE TABLE IF NOT EXISTS `phasardb`.`module_has_type_hierarchy` (
  `module_id` INT(11) NOT NULL,
  `type_hierarchy_id` INT(11) NOT NULL,
  PRIMARY KEY (`module_id`, `type_hierarchy_id`),
  INDEX `fk_module_has_type_hierarchy_type_hierarchy1_idx` (`type_hierarchy_id` ASC),
  INDEX `fk_module_has_type_hierarchy_module1_idx` (`module_id` ASC),
  CONSTRAINT `fk_module_has_type_hierarchy_module1`
    FOREIGN KEY (`module_id`)
    REFERENCES `phasardb`.`module` (`module_id`)
    ON DELETE CASCADE
    ON UPDATE CASCADE,
  CONSTRAINT `fk_module_has_type_hierarchy_type_hierarchy1`
    FOREIGN KEY (`type_hierarchy_id`)
    REFERENCES `phasardb`.`type_hierarchy` (`type_hierarchy_id`)
    ON DELETE CASCADE
    ON UPDATE CASCADE)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8
COLLATE = utf8_unicode_ci;

CREATE TABLE IF NOT EXISTS `phasardb`.`module_has_type` (
  `module_id` INT(11) NOT NULL,
  `type_id` INT(11) NOT NULL,
  PRIMARY KEY (`module_id`, `type_id`),
  INDEX `fk_module_has_type_type1_idx` (`type_id` ASC),
  INDEX `fk_module_has_type_module1_idx` (`module_id` ASC),
  CONSTRAINT `fk_module_has_type_module1`
    FOREIGN KEY (`module_id`)
    REFERENCES `phasardb`.`module` (`module_id`)
    ON DELETE CASCADE
    ON UPDATE CASCADE,
  CONSTRAINT `fk_module_has_type_type1`
    FOREIGN KEY (`type_id`)
    REFERENCES `phasardb`.`type` (`type_id`)
    ON DELETE CASCADE
    ON UPDATE CASCADE)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8
COLLATE = utf8_unicode_ci;

CREATE TABLE IF NOT EXISTS `phasardb`.`type_hierarchy_has_type` (
  `type_hierarchy_id` INT(11) NOT NULL,
  `type_id` INT(11) NOT NULL,
  PRIMARY KEY (`type_hierarchy_id`, `type_id`),
  INDEX `fk_type_hierarchy_has_type_type1_idx` (`type_id` ASC),
  INDEX `fk_type_hierarchy_has_type_type_hierarchy1_idx` (`type_hierarchy_id` ASC),
  CONSTRAINT `fk_type_hierarchy_has_type_type_hierarchy1`
    FOREIGN KEY (`type_hierarchy_id`)
    REFERENCES `phasardb`.`type_hierarchy` (`type_hierarchy_id`)
    ON DELETE CASCADE
    ON UPDATE CASCADE,
  CONSTRAINT `fk_type_hierarchy_has_type_type1`
    FOREIGN KEY (`type_id`)
    REFERENCES `phasardb`.`type` (`type_id`)
    ON DELETE CASCADE
    ON UPDATE CASCADE)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8
COLLATE = utf8_unicode_ci;

CREATE TABLE IF NOT EXISTS `phasardb`.`function_has_ifds_ide_summary` (
  `function_id` INT(11) NOT NULL,
  `ifds_ide_summary_id` INT(11) NOT NULL,
  PRIMARY KEY (`function_id`, `ifds_ide_summary_id`),
  INDEX `fk_function_has_ifds_ide_summary_ifds_ide_summary1_idx` (`ifds_ide_summary_id` ASC),
  INDEX `fk_function_has_ifds_ide_summary_function1_idx` (`function_id` ASC),
  CONSTRAINT `fk_function_has_ifds_ide_summary_function1`
    FOREIGN KEY (`function_id`)
    REFERENCES `phasardb`.`function` (`function_id`)
    ON DELETE CASCADE
    ON UPDATE CASCADE,
  CONSTRAINT `fk_function_has_ifds_ide_summary_ifds_ide_summary1`
    FOREIGN KEY (`ifds_ide_summary_id`)
    REFERENCES `phasardb`.`ifds_ide_summary` (`ifds_ide_summary_id`)
    ON DELETE CASCADE
    ON UPDATE CASCADE)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8
COLLATE = utf8_unicode_ci;

CREATE TABLE IF NOT EXISTS `phasardb`.`function_has_points-to_graph` (
  `function_id` INT(11) NOT NULL,
  `points-to_graph_id` INT(11) NOT NULL,
  PRIMARY KEY (`function_id`, `points-to_graph_id`),
  INDEX `fk_function_has_points-to_graph_points-to_graph1_idx` (`points-to_graph_id` ASC),
  INDEX `fk_function_has_points-to_graph_function1_idx` (`function_id` ASC),
  CONSTRAINT `fk_function_has_points-to_graph_function1`
    FOREIGN KEY (`function_id`)
    REFERENCES `phasardb`.`function` (`function_id`)
    ON DELETE CASCADE
    ON UPDATE CASCADE,
  CONSTRAINT `fk_function_has_points-to_graph_points-to_graph1`
    FOREIGN KEY (`points-to_graph_id`)
    REFERENCES `phasardb`.`points-to_graph` (`points-to_graph_id`)
    ON DELETE CASCADE
    ON UPDATE CASCADE)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8
COLLATE = utf8_unicode_ci;

CREATE TABLE IF NOT EXISTS `phasardb`.`type_has_virtual_function` (
  `type_id` INT(11) NOT NULL,
  `function_id` INT(11) NOT NULL,
  `vtable_index` INT(11) NULL DEFAULT NULL,
  PRIMARY KEY (`type_id`, `function_id`),
  INDEX `fk_type_has_function_function1_idx` (`function_id` ASC),
  INDEX `fk_type_has_function_type1_idx` (`type_id` ASC),
  CONSTRAINT `fk_type_has_function_type1`
    FOREIGN KEY (`type_id`)
    REFERENCES `phasardb`.`type` (`type_id`)
    ON DELETE CASCADE
    ON UPDATE CASCADE,
  CONSTRAINT `fk_type_has_function_function1`
    FOREIGN KEY (`function_id`)
    REFERENCES `phasardb`.`function` (`function_id`)
    ON DELETE CASCADE
    ON UPDATE CASCADE)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8
COLLATE = utf8_unicode_ci;


SET SQL_MODE=@OLD_SQL_MODE;
SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS;
SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS;
