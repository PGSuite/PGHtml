## PGHtml | Template engine using PostgreSQL

### [Download](https://pghtml.org/en/download/) ### 
### [Documentation](https://pghtml.org/en/documentation/) ### 
### [Example](https://pghtml.org/en/#example) ### 
  
### Description ###

Tool is designed to display on the site static information from database, which:
- rarely updates (e.g. dictionaries)
- preparation takes a long time
- does not require real-time relevance (e.g. TOP products)


PGHtml is a command line utility that creates HTML, JS, JSON and other types of files using data retrieved from a PostgreSQL database. The file is created from a source file in which tags and variables have been replaced


### Replacements ### 


*   **\<pghtml-sql\>** - SQL query result

*   **\<pghtml-include\>** - file contents

*   **${[variable]}** - variable value 


### Important qualities ###

*   **Check for updates** - if contents of resulting file have not updated, then the file is not overwritten and web server does not redeploy it
*   **Import** - support for importing other files (such as HTML fragment or JSON data)
*   **Logging variability** - there are different output options to log: processing, created and updated files, prefix
*   **Error handling** - if an error occurs (for example, when executing a SQL query), processing continues, extended information is output to log (stderr) and at end of execution, the tool returns the code of unsuccessful execution to OS. This allows you to configure monitoring of data updates on the site
*   **Selecting processed files** - possible processing of specific files, directories with recursion and setting file extensions. At same time, built-in variables are created that can be used in referencies and in SQL queries. Usually root directory of site is specified
*   **Variables** - support variables from the command line, built-in variables (directories, paths to files, etc.) and tag attributes when importing file 
*   **Unlimited nesting** - support including files inside an included file, redefining variables multiple times and using them inside another variable
*   **Conditions and loops** - programming inside files not supported, PostgreSQL stored procedures are used to implement logic
