// Copyright 2017 ESRI
//
// All rights reserved under the copyright laws of the United States
// and applicable international laws, treaties, and conventions.
//
// You may freely redistribute and use this sample code, with or
// without modification, provided you include the original copyright
// notice and use restrictions.
//
// See the Sample code usage restrictions document for further information.
//

// API
#include "ArcGISTiledElevationSource.h"
#include "Scene.h"
#include "ElevationSource.h"
#include "GeoView.h"
#include "FeatureLayer.h"
#include "RasterLayer.h"
#include "Geodatabase.h"
#include "GeodatabaseFeatureTable.h"
#include "FeatureTable.h"
#include "GeoPackage.h"
#include "GeoPackageFeatureTable.h"
#include "GeoPackageRaster.h"
#include "ShapefileFeatureTable.h"
#include "ArcGISSceneLayer.h"
#include "ArcGISTiledLayer.h"
#include "ArcGISVectorTiledLayer.h"
#include "Raster.h"
#include "RasterElevationSource.h"

// Toolkit
#include "AbstractTool.h"
#include "ToolManager.h"
#include "ToolResourceProvider.h"
#include "LayerCacheManager.h"

// Dsa apps
#include "AlertLevel.h"
#include "AlertConstants.h"
#include "DsaUtility.h"
#include "DsaController.h"
#include "MessageFeedConstants.h"

// Qt
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSettings>

using namespace Esri::ArcGISRuntime;

bool readJsonFile(QIODevice& device, QSettings::SettingsMap& map);
bool writeJsonFile(QIODevice& device, const QSettings::SettingsMap& map);

DsaController::DsaController(QObject* parent):
  QObject(parent),
  m_scene(new Scene(this)),
  m_jsonFormat(QSettings::registerFormat("json", &readJsonFile, &writeJsonFile)),
  m_conflictingToolNames{QStringLiteral("Alert Conditions"),
                         QStringLiteral("identify"),
                         QStringLiteral("Markup Tool"),
                         QStringLiteral("CoordinateConversion"),
                         QStringLiteral("analysis")}
{
  // setup config settings
  setupConfig();
  m_dataPath = m_dsaSettings["RootDataDirectory"].toString();

  connect(m_scene, &Scene::errorOccurred, this, &DsaController::onError);

  // set an elevation source
  RasterElevationSource* source = new RasterElevationSource(QStringList() << m_dsaSettings["DefaultElevationSource"].toStringList()[0], this);
  connect(source, &RasterElevationSource::errorOccurred, this, &DsaController::onError);
  m_scene->baseSurface()->elevationSources()->append(source);
}

DsaController::~DsaController()
{
  // save the settings
  saveSettings();
}

Esri::ArcGISRuntime::Scene* DsaController::scene() const
{
  return m_scene;
}

void DsaController::init(GeoView* geoView)
{
  Toolkit::ToolResourceProvider::instance()->setScene(m_scene);
  Toolkit::ToolResourceProvider::instance()->setGeoView(geoView);

  m_cacheManager = new LayerCacheManager(this);

  // connect all tool signals
  for(Toolkit::AbstractTool* abstractTool : Toolkit::ToolManager::instance())
  {
    if (!abstractTool)
      continue;

    connect(abstractTool, &Toolkit::AbstractTool::errorOccurred, this, &DsaController::onError);
    connect(abstractTool, &Toolkit::AbstractTool::propertyChanged, this, &DsaController::onPropertyChanged);

    if (abstractTool->metaObject()->indexOfSignal("toolErrorOccurred(QString,QString)") != -1)
      connect(abstractTool, SIGNAL(toolErrorOccurred(QString,QString)), this, SLOT(onToolError(QString, QString)));

    // certain tools can conflict - for example, those which interact directly with the view
    if (!isConflictingTool(abstractTool->toolName()))
      continue;

    // whenever a conflciting tool is activated, deactivate all of the other conflicting tools
    connect(abstractTool, &Toolkit::AbstractTool::activeChanged, this, [this, abstractTool]()
    {
      if (!abstractTool->isActive())
        return;

      auto toolsIt = Toolkit::ToolManager::instance().begin();
      auto toolsEnd = Toolkit::ToolManager::instance().end();
      for (; toolsIt != toolsEnd; ++toolsIt)
      {
        Toolkit::AbstractTool* candidateTool = *toolsIt;
        if (!candidateTool)
          continue;

        if (candidateTool->toolName() == abstractTool->toolName())
          continue;

        if (!isConflictingTool(candidateTool->toolName()))
          continue;

        candidateTool->setActive(false);
      }
    });
  }

  // set all tool properties
  for(Toolkit::AbstractTool* abstractTool : Toolkit::ToolManager::instance())
  {
    if (!abstractTool)
      continue;

    abstractTool->setProperties(m_dsaSettings);
  }
}

/*! \brief Slot to handle an ArcGISRuntime Error \a e.
 *
 */
void DsaController::onError(const Error& e)
{
  qDebug() << "Error" << e.message() << e.additionalMessage();
  emit errorOccurred(e.message(), e.additionalMessage());
}

/*! \brief Slot to handle an \a errorMessage (with an \a additionalMessage) from an \l AbstractTool.
 *
 */
void DsaController::onToolError(const QString& errorMessage, const QString& additionalMessage)
{
  qDebug() << "Error" << errorMessage << additionalMessage;
  emit errorOccurred(errorMessage, additionalMessage);
}

void DsaController::onPropertyChanged(const QString& propertyName, const QVariant& propertyValue)
{
  m_dsaSettings.insert(propertyName, propertyValue);
  // save the settings
  saveSettings();
}

void DsaController::setupConfig()
{
  // create the default settings map
  createDefaultSettings();

  // get the app config
  m_configFilePath = QString("%1/%2").arg(m_dsaSettings["RootDataDirectory"].toString(), QStringLiteral("DsaAppConfig.json"));

  // If the config file does not exist, create it, and set all of the defaults
  if (!QFileInfo::exists(m_configFilePath))
  {
    saveSettings();
  }
  else
  {
    // Open the config file, get settings, set them to the application controller
    QSettings settings(m_configFilePath, m_jsonFormat);
    const QStringList allKeys = settings.allKeys();

    // get the values from the config, and write to the settings map
    for (const QString& key : allKeys)
      m_dsaSettings[key] = settings.value(key);
  }
}

/*! \brief internal
 *
 * Writes the default initial location (in this app Monterey California)
 * as JSON to the settings map.
 */
void DsaController::writeDefaultInitialLocation()
{
  QJsonObject initialLocationJson;
  const QString centerString = DsaUtility::montereyCA().toJson();
  const QJsonDocument centerDoc = QJsonDocument::fromJson(centerString.toLatin1());
  initialLocationJson.insert( QStringLiteral("center"), centerDoc.object());
  initialLocationJson.insert( QStringLiteral("distance"), 5000.0);
  initialLocationJson.insert( QStringLiteral("heading"), 0.0);
  initialLocationJson.insert( QStringLiteral("pitch"), 75.0);
  initialLocationJson.insert( QStringLiteral("roll"), 0.0);
  m_dsaSettings[QStringLiteral("InitialLocation")] = initialLocationJson.toVariantMap();
}

/*! \brief internal
 *
 * Writes the default local data paths as JSON to the settings map.
 */
void DsaController::writeDefaultLocalDataPaths()
{
  const QString rootDir = m_dsaSettings["RootDataDirectory"].toString();
  QStringList pathsList{QString("%1/").arg(rootDir),
        QString("%1/OperationalData").arg(rootDir)};
  m_dsaSettings[QStringLiteral("LocalDataPaths")] = pathsList;
}

/*! \brief internal
 *
 * Writes the default Alert Conditions as JSON to the settings map.
 */
void DsaController::writeDefaultConditions()
{
  QJsonArray allConditionsJson;

  // Add a condition "Distress" when an object from the Friendly Tracks Land feed has attribute status911 = 1
  QJsonObject conditionJson;
  conditionJson.insert(AlertConstants::CONDITION_NAME, QStringLiteral("Distress"));
  conditionJson.insert(AlertConstants::CONDITION_LEVEL, static_cast<int>(AlertLevel::Critical));
  conditionJson.insert(AlertConstants::CONDITION_TYPE, AlertConstants::attributeEqualsAlertConditionType());
  conditionJson.insert(AlertConstants::CONDITION_SOURCE, QStringLiteral("Friendly Tracks - Land"));
  QJsonObject queryObject;
  queryObject.insert(AlertConstants::ATTRIBUTE_NAME, QStringLiteral("status911"));
  conditionJson.insert(AlertConstants::CONDITION_QUERY, queryObject);
  conditionJson.insert(AlertConstants::CONDITION_TARGET, "1");
  allConditionsJson.append(conditionJson);
  m_dsaSettings.insert(AlertConstants::ALERT_CONDITIONS_PROPERTYNAME, allConditionsJson.toVariantList());
}

/*! \brief internal
 *
 * Writes the default message feeds to the settings map.
 */
void DsaController::writeDefaultMessageFeeds()
{
  m_dsaSettings[MessageFeedConstants::MESSAGE_FEED_UDP_PORTS_PROPERTYNAME] = QStringList { QString("45678"), QString("45679") };

  QJsonArray messageFeedsJson;

  QJsonObject cotMessageFeedJson;
  cotMessageFeedJson.insert(MessageFeedConstants::MESSAGE_FEEDS_NAME, QStringLiteral("SA Events"));
  cotMessageFeedJson.insert(MessageFeedConstants::MESSAGE_FEEDS_TYPE, QStringLiteral("cot"));
  cotMessageFeedJson.insert(MessageFeedConstants::MESSAGE_FEEDS_RENDERER, QStringLiteral("mil2525c"));
  cotMessageFeedJson.insert(MessageFeedConstants::MESSAGE_FEEDS_THUMBNAIL, QStringLiteral("saevents.png"));
  cotMessageFeedJson.insert(MessageFeedConstants::MESSAGE_FEEDS_PLACEMENT, QStringLiteral("draped"));
  messageFeedsJson.append(cotMessageFeedJson);

  QJsonObject friendlyTracksLandJson;
  friendlyTracksLandJson.insert(MessageFeedConstants::MESSAGE_FEEDS_NAME, QStringLiteral("Friendly Tracks - Land"));
  friendlyTracksLandJson.insert(MessageFeedConstants::MESSAGE_FEEDS_TYPE, QStringLiteral("position_report_land"));
  friendlyTracksLandJson.insert(MessageFeedConstants::MESSAGE_FEEDS_RENDERER, QStringLiteral("mil2525c"));
  friendlyTracksLandJson.insert(MessageFeedConstants::MESSAGE_FEEDS_THUMBNAIL, QStringLiteral("friendlytracks.png"));
  friendlyTracksLandJson.insert(MessageFeedConstants::MESSAGE_FEEDS_PLACEMENT, QStringLiteral("draped"));
  messageFeedsJson.append(friendlyTracksLandJson);

  QJsonObject friendlyTracksAirJson;
  friendlyTracksAirJson.insert(MessageFeedConstants::MESSAGE_FEEDS_NAME, QStringLiteral("Friendly Tracks - Air"));
  friendlyTracksAirJson.insert(MessageFeedConstants::MESSAGE_FEEDS_TYPE, QStringLiteral("position_report_air"));
  friendlyTracksAirJson.insert(MessageFeedConstants::MESSAGE_FEEDS_RENDERER, QStringLiteral("mil2525c"));
  friendlyTracksAirJson.insert(MessageFeedConstants::MESSAGE_FEEDS_THUMBNAIL, QStringLiteral("friendlytracks-air.png"));
  friendlyTracksAirJson.insert(MessageFeedConstants::MESSAGE_FEEDS_PLACEMENT, QStringLiteral("absolute"));
  messageFeedsJson.append(friendlyTracksAirJson);

  QJsonObject spotRepJson;
  spotRepJson.insert(MessageFeedConstants::MESSAGE_FEEDS_NAME, QStringLiteral("Contact Reports"));
  spotRepJson.insert(MessageFeedConstants::MESSAGE_FEEDS_TYPE, QStringLiteral("spotrep"));
  spotRepJson.insert(MessageFeedConstants::MESSAGE_FEEDS_THUMBNAIL, QStringLiteral("enemycontact1600.png"));
  spotRepJson.insert(MessageFeedConstants::MESSAGE_FEEDS_RENDERER, QStringLiteral("enemycontact1600.png"));
  spotRepJson.insert(MessageFeedConstants::MESSAGE_FEEDS_PLACEMENT, QStringLiteral("draped"));
  messageFeedsJson.append(spotRepJson);

  QJsonObject sitRepJson;
  sitRepJson.insert(MessageFeedConstants::MESSAGE_FEEDS_NAME, QStringLiteral("Situation Reports"));
  sitRepJson.insert(MessageFeedConstants::MESSAGE_FEEDS_TYPE, QStringLiteral("sitrep"));
  sitRepJson.insert(MessageFeedConstants::MESSAGE_FEEDS_THUMBNAIL, QStringLiteral("sitrep1600.png"));
  sitRepJson.insert(MessageFeedConstants::MESSAGE_FEEDS_RENDERER, QStringLiteral("sitrep1600.png"));
  sitRepJson.insert(MessageFeedConstants::MESSAGE_FEEDS_PLACEMENT, QStringLiteral("draped"));
  messageFeedsJson.append(sitRepJson);

  QJsonObject eodReportsJson;
  eodReportsJson.insert(MessageFeedConstants::MESSAGE_FEEDS_NAME, QStringLiteral("EOD Reports"));
  eodReportsJson.insert(MessageFeedConstants::MESSAGE_FEEDS_TYPE, QStringLiteral("eod"));
  eodReportsJson.insert(MessageFeedConstants::MESSAGE_FEEDS_THUMBNAIL, QStringLiteral("eod1600.png"));
  eodReportsJson.insert(MessageFeedConstants::MESSAGE_FEEDS_RENDERER, QStringLiteral("eod1600.png"));
  eodReportsJson.insert(MessageFeedConstants::MESSAGE_FEEDS_PLACEMENT, QStringLiteral("draped"));
  messageFeedsJson.append(eodReportsJson);

  QJsonObject sensorObsJson;
  sensorObsJson.insert(MessageFeedConstants::MESSAGE_FEEDS_NAME, QStringLiteral("Sensor Observations"));
  sensorObsJson.insert(MessageFeedConstants::MESSAGE_FEEDS_TYPE, QStringLiteral("sensor_obs"));
  sensorObsJson.insert(MessageFeedConstants::MESSAGE_FEEDS_THUMBNAIL, QStringLiteral("sensorobs1600.png"));
  sensorObsJson.insert(MessageFeedConstants::MESSAGE_FEEDS_RENDERER, QStringLiteral("sensorobs1600.png"));
  sensorObsJson.insert(MessageFeedConstants::MESSAGE_FEEDS_PLACEMENT, QStringLiteral("draped"));
  messageFeedsJson.append(sensorObsJson);
  m_dsaSettings[MessageFeedConstants::MESSAGE_FEEDS_PROPERTYNAME] = messageFeedsJson;

  QJsonObject locationBroadcastJson;
  locationBroadcastJson.insert(MessageFeedConstants::LOCATION_BROADCAST_CONFIG_MESSAGE_TYPE, QStringLiteral("position_report_land"));
  locationBroadcastJson.insert(MessageFeedConstants::LOCATION_BROADCAST_CONFIG_PORT, 45679);
  m_dsaSettings[MessageFeedConstants::LOCATION_BROADCAST_CONFIG_PROPERTYNAME] = locationBroadcastJson;
}

/*! \brief internal
 *
 * Returns \c true if this tool is considered to conlfict with other tools.
 *
 * For example, this could be tools which interact directly with the view.
 */
bool DsaController::isConflictingTool(const QString& toolName) const
{
  return m_conflictingToolNames.contains(toolName);
}

/*! \brief internal
 *
 * This creates the default values for the config file. If the app
 * starts and there is no config file, it will create one, and write
 * the following values to the file.
 */
void DsaController::createDefaultSettings()
{
  // setup the defaults
  m_dsaSettings["RootDataDirectory"] = DsaUtility::dataPath();
  m_dsaSettings["BasemapDirectory"] = QString("%1/BasemapData").arg(m_dsaSettings["RootDataDirectory"].toString());
  m_dsaSettings["ElevationDirectory"] = QString("%1/ElevationData").arg(m_dsaSettings["RootDataDirectory"].toString());
  m_dsaSettings["SimulationDirectory"] = QString("%1/SimulationData").arg(m_dsaSettings["RootDataDirectory"].toString());
  m_dsaSettings["ResourceDirectory"] = QString("%1/ResourceData").arg(m_dsaSettings["RootDataDirectory"].toString());
  writeDefaultLocalDataPaths();
  m_dsaSettings["DefaultBasemap"] = QStringLiteral("topographic");
  m_dsaSettings["DefaultElevationSource"] = QString("%1/6p5.tif").arg(m_dsaSettings["OperationalData_SD"].toString());
  m_dsaSettings["GpxFile"] = QString("%1/MontereyMounted.gpx").arg(m_dsaSettings["SimulationDirectory"].toString());
  m_dsaSettings["SimulateLocation"] = QStringLiteral("true");
  writeDefaultMessageFeeds();
  writeDefaultInitialLocation();
  m_dsaSettings["CoordinateFormat"] = QStringLiteral("DMS");
  m_dsaSettings["UnitOfMeasurement"] = QStringLiteral("meters");
  m_dsaSettings["UseGpsForElevation"] = QStringLiteral("true");
  writeDefaultConditions();
}

/*!
 * \brief Save the app properties to a custom JSON QSettings file.
 */
void DsaController::saveSettings()
{
  QSettings settings(m_configFilePath, m_jsonFormat);

  auto it = m_dsaSettings.cbegin();
  auto itEnd = m_dsaSettings.cend();
  for (; it != itEnd; ++it)
    settings.setValue(it.key(), it.value());
}

/*! \brief Read method for custom QSettings JSON format
 *
 * Attempts to read the information in \a device in a JSON format
 * and stores the results in \a map.
 *
 * Returns \c true if the read was successful and \c false if not.
 */
bool readJsonFile(QIODevice& device, QSettings::SettingsMap& map)
{
  const QByteArray data = device.readAll();
  const QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

  const QJsonObject jsonObject = jsonDoc.object();

  map = jsonObject.toVariantMap();

  return !map.isEmpty();
}

/*! \brief Write method for custom QSettings JSON format
 *
 * Attempts to encode the information in \a map as valid JSON
 * and writes the results to \a device.
 *
 * Returns \c true if the write was successful and \c false if not.
 */
bool writeJsonFile(QIODevice& device, const QSettings::SettingsMap& map)
{
  const QJsonObject jsonObject = QJsonObject::fromVariantMap(map);

  if (jsonObject.isEmpty())
    return false;

  const QJsonDocument conditionsJsonDoc(jsonObject);
  const qint64 writtenBytes = device.write(conditionsJsonDoc.toJson(QJsonDocument::Indented));

  return writtenBytes != -1;
}

