# hpwallart
This development sponsored by www.wallquotes.com

HP WallArt Suite integration with Drupal 7 and Ubercart
This Drupal 7 module integrates the HP WallArt Suite (https://hpwallart.com/sign_in) with Drupal 7 and Ubercart.
It is still heavily in development mode and requires making manual changes to your Drupal database.
The 'base_url' that the wall art suite uses to form your links for the API is set as a system variable in the config settings at admin/misc/hpwallart. The generic launch page is created at /hpwallart. This integration does not currently support hosted content.

Database Changes:

Table: users
Add column hp_auth_token varchar(16) null=yes
ALTER TABLE  `users` ADD  `hp_auth_token` VARCHAR( 16 ) NULL DEFAULT NULL

Create table hpwallart_projects
Field                   Type          Null  Comments

id                      int(11)       No		Auto numeric. Primary key
name                    varchar(255)	No    (documentation had project_name but that does not seem to work)
width                   varchar(255)	Yes
height                  varchar(255)	Yes
user_id                 int(11)       No		Foreign key, related to the user table
scene (1...n)           varchar(255)	Yes		Each of the background images
element (1...n)         varchar(255)	Yes		Each of the vector objects to place on top of the background
price                   float(0.00) 	Yes		Base price for square meter
content_context_token   varchar(255)	Yes		Unique identifier to pass to designer by GET
path                    varchar(255)	Yes		Described as Project Folder in documentation
format                  varchar(255)	Yes		Format of the project SKU (WP | WA | CV)
state                   varchar(255)	Yes		State of the project in designer (EDITING | IN_CART | PURCHASED | DELETED) CHANGED FROM STATUS
has_pixelation_warning  tinyint(1)    Yes   Boolean true or false
