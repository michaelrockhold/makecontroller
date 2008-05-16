
#include "Properties.h"
#include <QDir>


Properties::Properties(MainWindow *mainWindow) : QDialog( 0 )
{
	this->mainWindow = mainWindow;
	setupUi(this);
	connect(buttonBox, SIGNAL(accepted()), this, SLOT(applyChanges()));
  load( ); // initialize
}

/*
	Read the project properties from the project profile, load them into
	the properties dialog and show it.
*/
bool Properties::loadAndShow( )
{
	load();
	show();
	return true;
}

/*
  Read the project's properties from the project file
  and load them into the UI.
*/
bool Properties::load()
{
  QDir projectDir(mainWindow->currentProjectPath());
	QString projectName = projectDir.dirName();
	setWindowTitle(projectName + " - Properties");
	
	// read the properties file
	QFile file(propFilePath());
	if(file.open(QIODevice::ReadOnly|QFile::Text))
	{
		QDomDocument propsFile;
    if(propsFile.setContent(&file))
		{
			versionEdit->setText(propsFile.elementsByTagName("version").at(0).toElement().text());
			heapSizeEdit->setText(propsFile.elementsByTagName("heapsize").at(0).toElement().text());
			QString optlevel = propsFile.elementsByTagName("optlevel").at(0).toElement().text();
			optLevelBox->setCurrentIndex(optLevelBox->findText(optlevel));
			Qt::CheckState state = (propsFile.elementsByTagName("debuginfo").at(0).toElement().text() == "true") ? Qt::Checked : Qt::Unchecked;
			debugInfoCheckbox->setCheckState(state);
		}
		file.close();
	}
	else
		return false;
  return true;
}

// rip through the fields and see if any have changed
// update appropriately if they have
void Properties::applyChanges( )
{
	QFile file(propFilePath());
	if(file.open(QIODevice::ReadWrite|QFile::Text))
	{
		QDomDocument propsFile;
    if(propsFile.setContent(&file))
    {
      // to get at the actual text of an element, you need to grab its child,
      // which will be a QDomText node
      if(versionEdit->text() != propsFile.elementsByTagName("version").at(0).toElement().text())
        propsFile.elementsByTagName("version").at(0).firstChild().setNodeValue(versionEdit->text());
        
      if(heapSizeEdit->text() != propsFile.elementsByTagName("heapsize").at(0).toElement().text())
        propsFile.elementsByTagName("heapsize").at(0).firstChild().setNodeValue(heapSizeEdit->text());
        
      if(optLevelBox->currentText() != propsFile.elementsByTagName("optlevel").at(0).toElement().text())
        propsFile.elementsByTagName("optlevel").at(0).firstChild().setNodeValue(optLevelBox->currentText());
      
      Qt::CheckState state = (propsFile.elementsByTagName("debuginfo").at(0).toElement().text() == "true") ? Qt::Checked : Qt::Unchecked;
      if(debugInfoCheckbox->checkState() != state)
      {
        QString debugstr = (debugInfoCheckbox->checkState() == Qt::Checked) ? "true" : "false";
        propsFile.elementsByTagName("debuginfo").at(0).firstChild().setNodeValue(debugstr);
      }
      file.resize(0); // clear out the current contents so we can update them
      file.write(propsFile.toByteArray(2));
    }
		file.close();
	}
}

/*
  Return the path of the project file
  for the current project.
*/
QString Properties::propFilePath( )
{
	QDir projectDir(mainWindow->currentProjectPath());
	QString projectName = projectDir.dirName();
	// filename should not have spaces
	return projectDir.filePath(projectName.remove(" ") + ".xml"); 
}

/*
  Return the optimization level
*/
QString Properties::optLevel()
{
  return optLevelBox->currentText();
}

/*
  Return whether or not to include debug info in this project's build.
*/ 
bool Properties::debug()
{
  if(debugInfoCheckbox->checkState() == Qt::Checked)
    return true;
  else
    return false;
}



