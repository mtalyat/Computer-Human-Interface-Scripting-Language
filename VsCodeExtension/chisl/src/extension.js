const vscode = require('vscode');
const fs = require('fs');
const path = require('path');

function activate(context) {
  context.subscriptions.push(
    vscode.commands.registerCommand('chisl.runChislFile', () => {
      const editor = vscode.window.activeTextEditor;
      if (editor) {
        const document = editor.document;
        const filePath = document.fileName;
        const workspaceFolders = vscode.workspace.workspaceFolders;

        if (workspaceFolders) {
          const workspaceRoot = workspaceFolders[0].uri.fsPath;
          const vscodeDir = path.join(workspaceRoot, '.vscode');
          const tasksJsonPath = path.join(vscodeDir, 'tasks.json');

          if (!fs.existsSync(vscodeDir)) {
            fs.mkdirSync(vscodeDir);
          }

          // Prepare the tasks.json content
          const tasksJsonContent = {
            "version": "2.0.0",
            "tasks": [
              {
                "label": "Run CHISL File",
                "type": "shell",
                "command": "chisl",
                "args": [filePath],
                "group": {
                  "kind": "build",
                  "isDefault": true
                },
                "presentation": {
                  "echo": true,
                  "reveal": "always",
                  "focus": false,
                  "panel": "shared"
                },
                "problemMatcher": []
              }
            ]
          };

          // Write the tasks.json file
          fs.writeFileSync(tasksJsonPath, JSON.stringify(tasksJsonContent, null, 2));

          // Run the task
          vscode.tasks.executeTask(new vscode.Task(
            { type: 'shell', task: 'Run CHISL File' },
            vscode.TaskScope.Workspace,
            'Run CHISL File',
            'shell',
            new vscode.ShellExecution("chisl", [filePath])
          ));
        } else {
          vscode.window.showErrorMessage('No workspace folder is open.');
        }
      } else {
        vscode.window.showErrorMessage('No active editor found.');
      }
    })
  );
}

exports.activate = activate;
