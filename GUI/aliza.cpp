//#define ALIZA_PRINT_COUNT_GL_OBJ
//#define ALIZA_PERF_COLLISION

#include "aliza.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QProcess>
#include <QTextStream>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QColorDialog>
#include <QDateTime>
#include <QEventLoop>
#include <string>
#include <array>
#ifdef ALIZA_PERF_COLLISION
#include <chrono>
#endif
#include "iconutils.h"
#include "commonutils.h"
#include "contourutils.h"
#include "srutils.h"
#include "dicomutils.h"
#include "spectroscopyutils.h"
#include "updateqtcommand.h"
#include "histogramgen.h"
#include "studyframewidget.h"
#include "studygraphicswidget.h"
#include "srwidget.h"
#include "loaddicom_t.h"
#include "loaddicom.h"
#include <mdcmReader.h>
#include <mdcmFile.h>
#include <mdcmDataSet.h>
#include <mdcmUIDGenerator.h>
#include <mdcmParseException.h>
#include "vectormath/scalar/vectormath.h"
#include "mmath.h"
#include <itkVersion.h>

// These flags are used only for diagnostic build sometimes
#ifndef _WIN32
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
//#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#pragma GCC diagnostic ignored "-Wcast-align"
#endif

#include "btBulletCollisionCommon.h"

#ifndef _WIN32
#pragma GCC diagnostic pop
#endif

namespace
{

static QMap<int, ImageVariant*> scene3dimages;
static QList<ImageVariant*> selected_images;
static QList<ImageVariant*> animation_images;
static QList<double> anim3d_times;

static btDefaultCollisionConfiguration * g_collisionConfiguration = nullptr;
static btCollisionDispatcher           * g_dispatcher = nullptr;
static btDbvtBroadphase                * g_broadphase = nullptr;
static btCollisionWorld                * g_collisionWorld = nullptr;
static btAlignedObjectArray<btCollisionShape*> g_collision_shapes;
static bool show_all_study_collisions = true;

inline void delay1(int ms) // milliseconds
{
	QEventLoop loop;
	QTimer t;
	t.connect(&t, SIGNAL(timeout()), &loop, SLOT(quit()));
	t.start(ms);
	loop.exec();
}

struct ClosestRayResultCallback1 : public btCollisionWorld::ClosestRayResultCallback
{
	ClosestRayResultCallback1 (const btVector3 & rayFrom,const btVector3 & rayTo)
		: btCollisionWorld::ClosestRayResultCallback(rayFrom, rayTo) {}
	virtual ~ClosestRayResultCallback1() {}
	bool needsCollision(btBroadphaseProxy * proxy0) const
	{
		btCollisionObject * b =
			static_cast<btCollisionObject *>(proxy0->m_clientObject);
		const int * p = static_cast<int*>(b->getUserPointer());
		if (p && (p[2] == 2)) return true;
		return false;
	}
};

void search_frame_of_ref(
	const int id,
	const QString & frame_uid,
	const QString & study_uid,
	QList<const ImageVariant*> & l)
{
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	QMap<int, ImageVariant*>::const_iterator it = scene3dimages.cbegin();
	while (it != scene3dimages.cend())
#else
	QMap<int, ImageVariant*>::const_iterator it = scene3dimages.constBegin();
	while (it != scene3dimages.constEnd())
#endif
	{
		const ImageVariant * v = it.value();
		if (v &&
			(v->id != id) &&
			(v->frame_of_ref_uid == frame_uid) &&
			(v->study_uid == study_uid) &&
			(v->image_type < 100))
		{
			l.push_back(v);
		}
		++it;
	}
}

void search_frame_of_ref2(
	const int id,
	const QString & frame_uid,
	const QString & study_uid,
	QList<const ImageVariant*> & l)
{
	for (int x = 0; x < selected_images.size(); ++x)
	{
		const ImageVariant * v = selected_images.at(x);
		if (v &&
			(v->id != id) &&
			(v->frame_of_ref_uid == frame_uid) &&
			(v->study_uid == study_uid) &&
			(v->image_type < 100))
		{
			l.push_back(v);
		}
	}
}

void g_init_physics()
{
	g_collisionConfiguration = new btDefaultCollisionConfiguration();
	g_dispatcher = new btCollisionDispatcher(g_collisionConfiguration);
	g_broadphase = new btDbvtBroadphase();
	g_collisionWorld = new btCollisionWorld(g_dispatcher,g_broadphase,g_collisionConfiguration);
#if 0
	std::cout << "Bullet " << btGetVersion() << std::endl;
#endif
}

void g_close_physics()
{
	if (g_collisionWorld)
	{
		for (int x = g_collisionWorld->getNumCollisionObjects() - 1; x >= 0; --x)
		{
			btCollisionObject * o = g_collisionWorld->getCollisionObjectArray()[x];
			if (o)
			{
				if (o->getUserPointer())
				{
					int * p = static_cast<int*>(o->getUserPointer());
					delete [] p;
				}
				g_collisionWorld->removeCollisionObject(o);
				delete o;
				o = nullptr;
			}
		}
	}
	const int g_collision_shapes_size = g_collision_shapes.size();
	if (g_collision_shapes_size > 0)
	{
		for (int x = 0; x < g_collision_shapes_size; ++x)
		{
			btCollisionShape * s = g_collision_shapes[x];
			if (s)
			{
				if (s->getShapeType() == COMPOUND_SHAPE_PROXYTYPE)
				{
					btCompoundShape * c = static_cast<btCompoundShape*>(s);
					for (int z = 0; z < c->getNumChildShapes(); ++z)
					{
						btCollisionShape * ch = c->getChildShape(z);
						if (ch)
						{
							c->removeChildShape(ch);
							delete ch;
							ch = nullptr;
						}
					}
				}
				delete s;
				s = nullptr;
			}
		}
		g_collision_shapes.clear();
	}
	if (g_collisionWorld)
	{
		delete g_collisionWorld;
		g_collisionWorld = nullptr;
	}
	if (g_collisionConfiguration)
	{
		delete g_collisionConfiguration;
		g_collisionConfiguration = nullptr;
	}
	if (g_dispatcher)
	{
		delete g_dispatcher;
		g_dispatcher = nullptr;
	}
	if (g_broadphase)
	{
		delete g_broadphase;
		g_broadphase = nullptr;
	}
#if 0
	std::cout << "Bullet closed" << std::endl;
#endif
}

bool check_slices_parallel(
	const ImageVariant * v0,
	const int z0,
	const ImageVariant * v1,
	const int z1)
{
	const float px0 = v0->di->image_slices.at(z0)->v[0];
	const float py0 = v0->di->image_slices.at(z0)->v[1];
	const float pz0 = v0->di->image_slices.at(z0)->v[2];
	const Vectormath::Scalar::Vector3 v0v1(
		v0->di->image_slices.at(z0)->v[3] - px0,
		v0->di->image_slices.at(z0)->v[4] - py0,
		v0->di->image_slices.at(z0)->v[5] - pz0);
	const Vectormath::Scalar::Vector3 v0v2(
		v0->di->image_slices.at(z0)->v[6] - px0,
		v0->di->image_slices.at(z0)->v[7] - py0,
		v0->di->image_slices.at(z0)->v[8] - pz0);
	const Vectormath::Scalar::Vector3 n0 =
		Vectormath::Scalar::normalize(Vectormath::Scalar::cross(v0v1,v0v2));
	const float px1 = v1->di->image_slices.at(z1)->v[0];
	const float py1 = v1->di->image_slices.at(z1)->v[1];
	const float pz1 = v1->di->image_slices.at(z1)->v[2];
	const Vectormath::Scalar::Vector3 v1v1(
		v1->di->image_slices.at(z1)->v[3] - px1,
		v1->di->image_slices.at(z1)->v[4] - py1,
		v1->di->image_slices.at(z1)->v[5] - pz1);
	const Vectormath::Scalar::Vector3 v1v2(
		v1->di->image_slices.at(z1)->v[6] - px1,
		v1->di->image_slices.at(z1)->v[7] - py1,
		v1->di->image_slices.at(z1)->v[8] - pz1);
	const Vectormath::Scalar::Vector3 n1 =
		Vectormath::Scalar::normalize(Vectormath::Scalar::cross(v1v1, v1v2));
	if ((
		(MMath::AlmostEqual(n0.getX(), n1.getX())) &&
		(MMath::AlmostEqual(n0.getY(), n1.getY())) &&
		(MMath::AlmostEqual(n0.getZ(), n1.getZ())))
		||
		(
		(MMath::AlmostEqual(n0.getX(), -n1.getX())) &&
		(MMath::AlmostEqual(n0.getY(), -n1.getY())) &&
		(MMath::AlmostEqual(n0.getZ(), -n1.getZ()))))
	{
		return true;
	}
	return false;
}

void add_slice_collision_plane(
	const ImageVariant * v,
	const int z,
	const int id,
	btAlignedObjectArray<btStaticPlaneShape*> & tmp_shapes,
	btAlignedObjectArray<btCollisionObject*> & tmp_objects)
{
	btTransform t;
	t.setIdentity();
	const float px = v->di->image_slices.at(z)->v[0];
	const float py = v->di->image_slices.at(z)->v[1];
	const float pz = v->di->image_slices.at(z)->v[2];
	const Vectormath::Scalar::Vector3 v1(
		v->di->image_slices.at(z)->v[3] - px,
		v->di->image_slices.at(z)->v[4] - py,
		v->di->image_slices.at(z)->v[5] - pz);
	const Vectormath::Scalar::Vector3 v2(
		v->di->image_slices.at(z)->v[6] - px,
		v->di->image_slices.at(z)->v[7] - py,
		v->di->image_slices.at(z)->v[8] - pz);
	const Vectormath::Scalar::Vector3 n =
		Vectormath::Scalar::normalize(Vectormath::Scalar::cross(v1, v2));
	btStaticPlaneShape * s =
		new btStaticPlaneShape(btVector3(n.getX(), n.getY(), n.getZ()), 0);
	tmp_shapes.push_back(s);
	btCollisionObject * o = new btCollisionObject();
	o->setCollisionShape(s);
	t.setOrigin(btVector3(px, py, pz));
	o->setWorldTransform(t);
	int * usrP = new int[3];
	usrP[0] = v->di->id;
	usrP[1] = z;
	usrP[2] = id;
	o->setUserPointer(usrP);
	tmp_objects.push_back(o);
	g_collisionWorld->addCollisionObject(o);
}

void check_slice_collisions(const ImageVariant * v, GraphicsWidget * w)
{
#ifdef ALIZA_PERF_COLLISION
	const auto t0 = std::chrono::steady_clock::now();
#endif
	if (!w) return;
	if (w->get_axis() != 2) return;
	w->graphicsview->clear_collision_paths();
	if (!show_all_study_collisions) return;
	if (!g_collisionWorld) return;
	if (!v) return;
#if 1
	if (v->frame_of_ref_uid.isEmpty()) return;
#endif
	const int z = v->di->selected_z_slice;
	const int slices_size = v->di->image_slices.size();
	if (v->di->idimz != slices_size) return;
	if (z >= slices_size) return;
	QList<const ImageVariant*> refs;
	if (selected_images.size() > 1)
	{
		search_frame_of_ref2(v->id, v->frame_of_ref_uid, v->study_uid, refs);
	}
	else
	{
		search_frame_of_ref(v->id, v->frame_of_ref_uid, v->study_uid, refs);
	}
	if (refs.empty()) return;
	btAlignedObjectArray<btStaticPlaneShape*> tmp_shapes;
	btAlignedObjectArray<btCollisionObject*> tmp_objects;
	tmp_shapes.resize(0);
	tmp_objects.resize(0);
	add_slice_collision_plane(
		v,
		z,
		2,
		tmp_shapes,
		tmp_objects);
#if 1
	// TODO check, maybe computeOverlappingPairs() is enough
	// or nothing?
	g_collisionWorld->performDiscreteCollisionDetection();
#endif
	for (int u = 0; u < refs.size(); ++u)
	{
		const int z1 = refs.at(u)->di->selected_z_slice;
		if (z1 >= static_cast<int>(refs.at(u)->di->image_slices.size()))
		{
			continue;
		}
		if (check_slices_parallel(v, z, refs.at(u), z1))
		{
			continue;
		}
		std::vector< std::array<float, 2> > hits;
		for (int q = 0; q < 4; ++q)
		{
			int k0 = 0, k1 = 1, k2 = 2, k3 = 3, k4 = 4, k5 = 5;
			switch (q)
			{
			case 1:
				k0 = 3;
				k1 = 4;
				k2 = 5;
				k3 = 6;
				k4 = 7;
				k5 = 8;
				break;
			case 2:
				k0 = 6;
				k1 = 7;
				k2 = 8;
				k3 = 9;
				k4 = 10;
				k5 = 11;
				break;
			case 3:
				k0 = 9;
				k1 = 10;
				k2 = 11;
				k3 = 0;
				k4 = 1;
				k5 = 2;
				break;
			default:
				break;
			}
			const btVector3 from(
				refs.at(u)->di->image_slices.at(z1)->fv[k0],
				refs.at(u)->di->image_slices.at(z1)->fv[k1],
				refs.at(u)->di->image_slices.at(z1)->fv[k2]);
			const btVector3 to(
				refs.at(u)->di->image_slices.at(z1)->fv[k3],
				refs.at(u)->di->image_slices.at(z1)->fv[k4],
				refs.at(u)->di->image_slices.at(z1)->fv[k5]);
			ClosestRayResultCallback1 rayResult(from, to);
			g_collisionWorld->rayTest(from, to, rayResult);
			if (rayResult.hasHit())
			{
				const btCollisionObject * o = rayResult.m_collisionObject;
				if (o)
				{
					const int * usrP = static_cast<int*>(o->getUserPointer());
					if (usrP && usrP[0] == v->id)
					{
						const btVector3 hit = rayResult.m_hitPointWorld;
						ImageTypeUC::Pointer image = ImageTypeUC::New();
						const bool image_ok = ContourUtils::phys_space_from_slice(v, z, image);
						if (image_ok)
						{
							itk::Point<float, 3> point;
							point[0] = hit.getX();
							point[1] = hit.getY();
							point[2] = hit.getZ();
#if ITK_VERSION_MAJOR >= 5
							const auto index =
								image->template TransformPhysicalPointToContinuousIndex<float>(point);
#else
							itk::ContinuousIndex<float, 3> index;
							image->TransformPhysicalPointToContinuousIndex(point, index);
#endif
							hits.push_back({ { index[0], index[1] } });
						}
					}
				}
			}
		}
		if (hits.size() == 2)
		{
			const int R = static_cast<int>(refs.at(u)->di->R * 255.0f);
			const int G = static_cast<int>(refs.at(u)->di->G * 255.0f);
			const int B = static_cast<int>(refs.at(u)->di->B * 255.0f);
			QPen pen;
			pen.setBrush(QBrush(QColor(R, G, B, 255)));
			pen.setStyle(Qt::SolidLine);
			pen.setWidth(0);
			QPainterPath pp;
			pp.moveTo(hits.at(0).at(0), hits.at(0).at(1));
			pp.lineTo(hits.at(1).at(0), hits.at(1).at(1));
			QGraphicsPathItem * g = new QGraphicsPathItem();
			g->setPen(pen);
			g->setPath(pp);
			w->graphicsview->scene()->addItem(g);
			w->graphicsview->collision_paths.push_back(g);
		}
	}
	for (int j = 0; j < tmp_objects.size(); ++j)
	{
		if (tmp_objects[j])
		{
			btCollisionObject * k = tmp_objects[j];
			if (k->getUserPointer())
			{
				int * p = static_cast<int*>(k->getUserPointer());
				delete [] p;
			}
			g_collisionWorld->removeCollisionObject(k);
			delete tmp_objects[j];
			tmp_objects[j] = nullptr;
		}
	}
	for (int j = 0; j < tmp_shapes.size(); ++j)
	{
		if (tmp_shapes[j])
		{
			delete tmp_shapes[j];
			tmp_shapes[j] = nullptr;
		}
	}
#ifdef ALIZA_PERF_COLLISION
	const auto t1 = std::chrono::steady_clock::now();
	const auto ts = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0);
	std::cout << ts.count() << " ns" << std::endl;
#endif
}

void check_slice_collisions2(StudyViewWidget * w)
{
#ifdef ALIZA_PERF_COLLISION
	const auto t0 = std::chrono::steady_clock::now();
#endif
	if (!w) return;
	if (!g_collisionWorld) return;
	const int widgets_size = w->widgets.size();
	for (int x = 0; x < widgets_size; ++x)
	{
		if (w->widgets.at(x) && w->widgets.at(x)->graphicswidget)
		{
			w->widgets[x]->graphicswidget->graphicsview->clear_collision_paths();
		}
	}
	for (int x = 0; x < widgets_size; ++x)
	{
		if (w->widgets.at(x) && w->widgets.at(x)->graphicswidget)
		{
			if (w->widgets.at(x)->graphicswidget->image_container.image3D)
			{
				const ImageVariant * v = w->widgets.at(x)->graphicswidget->image_container.image3D;
				if (!v) continue;
				if (v->frame_of_ref_uid.isEmpty()) continue;
				const int slices_size = v->di->image_slices.size();
				if (v->di->idimz != slices_size) continue;
				const int z =
					w->widgets.at(x)->graphicswidget->image_container.selected_z_slice_ext;
				if (z >= slices_size) continue;
				btAlignedObjectArray<btStaticPlaneShape*> tmp_shapes;
				btAlignedObjectArray<btCollisionObject*> tmp_objects;
				tmp_shapes.resize(0);
				tmp_objects.resize(0);
				add_slice_collision_plane(
					v,
					z,
					2,
					tmp_shapes,
					tmp_objects);
#if 1
				// TODO check
				g_collisionWorld->performDiscreteCollisionDetection();
#endif
				for (int u = 0; u < widgets_size; ++u)
				{
					if (!(w->widgets.at(u) && w->widgets.at(u)->graphicswidget)) continue;
					const ImageVariant * v1 = w->widgets.at(u)->graphicswidget->image_container.image3D;
					if (!v1) continue;
					if (v->id == v1->id) continue;
					if (!((v->study_uid == v1->study_uid) && (v->frame_of_ref_uid == v1->frame_of_ref_uid)))
					{
						continue;
					}
					const int z1 = w->widgets.at(u)->graphicswidget->image_container.selected_z_slice_ext;
					const int v1_slices_size = v1->di->image_slices.size();
					if (v1->di->idimz != v1_slices_size) continue;
					if (z1 >= v1_slices_size) continue;
					if (check_slices_parallel(v, z, v1, z1)) continue;
					std::vector< std::array<float, 2> > hits;
					for (int q = 0; q < 4; ++q)
					{
						int k0 = 0, k1 = 1, k2 = 2, k3 = 3, k4 = 4, k5 = 5;
						switch (q)
						{
						case 1:
							k0 = 3;
							k1 = 4;
							k2 = 5;
							k3 = 6;
							k4 = 7;
							k5 = 8;
							break;
						case 2:
							k0 = 6;
							k1 = 7;
							k2 = 8;
							k3 = 9;
							k4 = 10;
							k5 = 11;
							break;
						case 3:
							k0 = 9;
							k1 = 10;
							k2 = 11;
							k3 = 0;
							k4 = 1;
							k5 = 2;
							break;
						default:
							break;
						}
						const btVector3 from(
							v1->di->image_slices.at(z1)->fv[k0],
							v1->di->image_slices.at(z1)->fv[k1],
							v1->di->image_slices.at(z1)->fv[k2]);
						const btVector3 to(
							v1->di->image_slices.at(z1)->fv[k3],
							v1->di->image_slices.at(z1)->fv[k4],
							v1->di->image_slices.at(z1)->fv[k5]);
						ClosestRayResultCallback1 rayResult(from, to);
						g_collisionWorld->rayTest(from, to, rayResult);
						if (rayResult.hasHit())
						{
							const btCollisionObject * o = rayResult.m_collisionObject;
							if (o)
							{
								const int * usrP = static_cast<int*>(o->getUserPointer());
								if (usrP && usrP[0] == v->id)
								{
									const btVector3 hit = rayResult.m_hitPointWorld;
									ImageTypeUC::Pointer image = ImageTypeUC::New();
									const bool image_ok = ContourUtils::phys_space_from_slice(v, z, image);
									if (image_ok)
									{
										itk::Point<float, 3> point;
										point[0] = hit.getX();
										point[1] = hit.getY();
										point[2] = hit.getZ();
#if ITK_VERSION_MAJOR >= 5
										const auto index =
											image->template TransformPhysicalPointToContinuousIndex<float>(point);
#else
										itk::ContinuousIndex<float, 3> index;
										image->TransformPhysicalPointToContinuousIndex(point, index);
#endif
										hits.push_back({ { index[0], index[1] } });
									}
								}
							}
						}
					}
					if (hits.size() == 2)
					{
						const int R = static_cast<int>(v1->di->R * 255.0f);
						const int G = static_cast<int>(v1->di->G * 255.0f);
						const int B = static_cast<int>(v1->di->B * 255.0f);
						QPen pen;
						pen.setBrush(QBrush(QColor(R, G, B, 255)));
						pen.setStyle(Qt::SolidLine);
						pen.setWidth(0);
						QPainterPath pp;
						pp.moveTo(hits.at(0).at(0), hits.at(0).at(1));
						pp.lineTo(hits.at(1).at(0), hits.at(1).at(1));
						QGraphicsPathItem * g = new QGraphicsPathItem();
						g->setPen(pen);
						g->setPath(pp);
						w->widgets[x]->graphicswidget->graphicsview->scene()->addItem(g);
						w->widgets[x]->graphicswidget->graphicsview->collision_paths.push_back(g);
					}
				}
				for (int j = 0; j < tmp_objects.size(); ++j)
				{
					if (tmp_objects[j])
					{
						btCollisionObject * k = tmp_objects[j];
						if (k->getUserPointer())
						{
							int * p = static_cast<int*>(k->getUserPointer());
							delete [] p;
						}
						g_collisionWorld->removeCollisionObject(k);
						delete tmp_objects[j];
						tmp_objects[j] = nullptr;
					}
				}
				for (int j = 0; j < tmp_shapes.size(); ++j)
				{
					if (tmp_shapes[j])
					{
						delete tmp_shapes[j];
						tmp_shapes[j] = nullptr;
					}
				}
			}
		}
	}
#ifdef ALIZA_PERF_COLLISION
	const auto t1 = std::chrono::steady_clock::now();
	const auto ts = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0);
	std::cout << ts.count() << " ns" << std::endl;
#endif
}

}

Aliza::Aliza()
{
	trans_icon = QIcon(QString(":/bitmaps/trans1.svg"));
	notrans_icon = QIcon(QString(":/bitmaps/notrans1.svg"));
	cut_icon = QIcon(QString(":bitmaps/cut.svg"));
	nocut_icon = QIcon(QString(":bitmaps/nocut.svg"));
	anchor_icon = QIcon(QString(":/bitmaps/anchor.svg"));
	anchor2_icon = QIcon(QString(":/bitmaps/anchor2.svg"));
	anim3D_timer = new QTimer();
	g_init_physics();
#if 1
	CommonUtils::save_total_memory();
#endif
}

Aliza::~Aliza()
{
	if (anim3D_timer->isActive()) anim3D_timer->stop();
	delete anim3D_timer;
}

void Aliza::close_()
{
	const bool ok3d = check_3d();
	if (ok3d) glwidget->set_skip_draw(true);
	stop_anim();
	stop_3D_anim();
	disconnect_tools();
	imagesbox->listWidget->clear();
	graphicswidget_m->clear_();
	graphicswidget_y->clear_();
	graphicswidget_x->clear_();
	if (!selected_images.empty()) selected_images.clear();
	if (!scene3dimages.empty())
	{
		QMap<int, ImageVariant*>::iterator iv = scene3dimages.begin();
		while (iv != scene3dimages.end())
		{
			ImageVariant * v = iv.value();
			++iv;
			delete v;
		}
		scene3dimages.clear();
	}
	if (ok3d) glwidget->close_();
	g_close_physics();
}

QString Aliza::load_dicom_series(QProgressDialog * pb)
{
	if (lock0) return QString("");
	lock0 = true;
	QString message;
	std::vector<ImageVariant*> ivariants;
	QStringList pdf_files;
	QStringList stl_files;
	QStringList video_files;
	QStringList spectroscopy_files;
	QStringList sr_files;
	std::vector<int> rows;
	QStringList filenames;
	const bool ok3d = check_3d();
	if (ok3d)
	{
		glwidget->set_skip_draw(true);
	}
	const bool dcm_thread = settingswidget->get_dcm_thread();
	const QModelIndexList selection =
		browser2->tableWidget->selectionModel()->selectedRows();
	for (int x = 0; x < selection.count(); ++x)
	{
		const QModelIndex index = selection.at(x);
		rows.push_back(index.row());
	}
	if (rows.empty())
	{
		lock0 = false;
		return QString("");
	}
	const QString root = browser2->get_root();
	for (unsigned int x = 0; x < rows.size(); ++x)
	{
		const int row = rows.at(x);
		if (row < 0) continue;
		const TableWidgetItem * item =
			static_cast<TableWidgetItem *>(browser2->tableWidget->item(row, 0));
		if (!item) continue;
		if ((item->files.empty())) continue;
		filenames = item->files;
		if (dcm_thread)
		{
			LoadDicom_T * lt = new LoadDicom_T(
				root,
				filenames,
				ok3d,
				static_cast<const QWidget * const>(
					const_cast<const SettingsWidget * const>(settingswidget)),
				0,
				settingswidget->get_enh_strategy());
			lt->start();
			while (!lt->isFinished())
			{
				delay1(100);
			}
			const QString message_ = lt->message;
			if (!message_.isEmpty())
			{
				const int filenames_size = filenames.size();
				if (!message.isEmpty()) message.append(QChar('\n'));
				if (filenames_size == 1)
				{
					message.append(filenames.at(0) + QString(":\n    "));
				}
				else if (filenames_size >= 1)
				{
					message.append(filenames.at(0) + QString(" (1st file):\n    "));
				}
				message.append(message_ + QString("\n\n"));
			}
			for (size_t k = 0; k < lt->ivariants.size(); ++k)
			{
				ivariants.push_back(lt->ivariants[k]);
			}
			for (int k = 0; k < lt->pdf_files.size(); ++k)
			{
				pdf_files.push_back(lt->pdf_files.at(k));
			}
			for (int k = 0; k < lt->stl_files.size(); ++k)
			{
				stl_files.push_back(lt->stl_files.at(k));
			}
			for (int k = 0; k < lt->video_files.size(); ++k)
			{
				video_files.push_back(lt->video_files.at(k));
			}
			for (int k = 0; k < lt->spectroscopy_files.size(); ++k)
			{
				spectroscopy_files.push_back(lt->spectroscopy_files.at(k));
			}
			for (int k = 0; k < lt->sr_files.size(); ++k)
			{
				sr_files.push_back(lt->sr_files.at(k));
			}
			delete lt;
		}
		else
		{
			LoadDicom * lt = new LoadDicom(
				root,
				filenames,
				ok3d,
				static_cast<const QWidget * const>(
					const_cast<const SettingsWidget * const>(settingswidget)),
				0,
				settingswidget->get_enh_strategy());
			lt->run();
			const QString message_ = lt->message;
			if (!message_.isEmpty())
			{
				const int filenames_size = filenames.size();
				if (!message.isEmpty()) message.append(QChar('\n'));
				if (filenames_size == 1)
				{
					message.append(filenames.at(0) + QString(":\n    "));
				}
				else if (filenames_size >= 1)
				{
					message.append(filenames.at(0) + QString(" (1st file):\n    "));
				}
				message.append(message_ + QString("\n\n"));
			}
			for (size_t k = 0; k < lt->ivariants.size(); ++k)
			{
				ivariants.push_back(lt->ivariants[k]);
			}
			for (int k = 0; k < lt->pdf_files.size(); ++k)
			{
				pdf_files.push_back(lt->pdf_files.at(k));
			}
			for (int k = 0; k < lt->stl_files.size(); ++k)
			{
				stl_files.push_back(lt->stl_files.at(k));
			}
			for (int k = 0; k < lt->video_files.size(); ++k)
			{
				video_files.push_back(lt->video_files.at(k));
			}
			for (int k = 0; k < lt->spectroscopy_files.size(); ++k)
			{
				spectroscopy_files.push_back(lt->spectroscopy_files.at(k));
			}
			for (int k = 0; k < lt->sr_files.size(); ++k)
			{
				sr_files.push_back(lt->sr_files.at(k));
			}
			delete lt;
		}
	}
	//
	const QString message_ = process_dicom(
		ivariants,
		pdf_files,
		stl_files,
		video_files,
		spectroscopy_files,
		sr_files,
		pb);
	if (!message_.isEmpty())
	{
		message.append(message_);
	}
	lock0 = false;
	return message;
}

void Aliza::add_histogram(ImageVariant * v, QProgressDialog * pb, bool check_settings)
{
	if (!v) return;
	if (v->image_type <  0) return;
	if (v->image_type > 16) return;
	if (check_settings) return; // always 'false'
	if (pb) pb->setLabelText(QString("Calculating histogram"));
	HistogramGen * t = new HistogramGen(v);
	t->start();
	while (!t->isFinished())
	{
		delay1(100);
	}
	t->gen_pixmap();
	const QString tmp0 = t->get_error();
#ifdef ALIZA_VERBOSE
	if (!tmp0.isEmpty())
	{
		std::cout << tmp0.toStdString() << std::endl;
	}
#else
	(void)tmp0;
#endif
	delete t;
}

void Aliza::clear_ram()
{
	if (lock0) return;
	lock0 = true;
	const bool ok3d = check_3d();
	if (ok3d) glwidget->set_skip_draw(true);
	graphicswidget_m->clear_();
	graphicswidget_y->clear_();
	graphicswidget_x->clear_();
	histogramview->clear__();
	if (studyview)
	{
		studyview->clear_();
		if (studyview->get_in_tab()) studyview->calculate_grid(2);
	}
	imagesbox->listWidget->blockSignals(true);
	disconnect(imagesbox->listWidget,SIGNAL(itemSelectionChanged()),this,SLOT(update_selection()));
	disconnect(imagesbox->listWidget,SIGNAL(itemChanged(QListWidgetItem*)),this,SLOT(update_selection()));
	imagesbox->listWidget->clear();
	imagesbox->set_html(nullptr);
	clear_contourstable();
	selected_images.clear();
	QMap<int, ImageVariant*>::iterator iv = scene3dimages.begin();
	while (iv != scene3dimages.end())
	{
		if (iv.value())
		{
			ImageVariant * tmp = iv.value();
			delete tmp;
		}
		++iv;
	}
	scene3dimages.clear();
	connect(imagesbox->listWidget,SIGNAL(itemSelectionChanged()),this,SLOT(update_selection()));
	connect(imagesbox->listWidget,SIGNAL(itemChanged(QListWidgetItem*)),this,SLOT(update_selection()));
	imagesbox->listWidget->blockSignals(false);
	//
	lock0 = false;
	if (ok3d) glwidget->set_skip_draw(false);
#ifdef ALIZA_PRINT_COUNT_GL_OBJ
	std::cout << "Num VBOs " << GLWidget::get_count_vbos() << std::endl;
#endif
}

void Aliza::delete_image()
{
	ImageVariant * ivariant{};
	QListWidgetItem * item__{};
	if (lock0) return;
	lock0 = true;
	const bool ok3d = check_3d();
	if (ok3d) glwidget->set_skip_draw(true);
	if (imagesbox->listWidget->selectedItems().empty()) goto quit__;
	ivariant = get_selected_image();
	if (!ivariant) goto quit__;
#if 1
	if (ivariant->group_id >= 0 && scene3dimages.size() > 1)
	{
		QMessageBox mbox;
		mbox.addButton(QMessageBox::Yes);
		mbox.addButton(QMessageBox::No);
		mbox.addButton(QMessageBox::Cancel);
		mbox.setDefaultButton(QMessageBox::No);
		mbox.setIcon(QMessageBox::Question);
		mbox.setText(QString("Close all images of 4D group?"));
		const int q = mbox.exec();
		if (q == QMessageBox::Cancel)
		{
			goto quit__;
		}
		else if (q == QMessageBox::Yes)
		{
			QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
			delete_group(ivariant->group_id);
			QApplication::restoreOverrideCursor();
			goto quit__;
		}
	}
#endif
	item__ = imagesbox->listWidget->selectedItems()[0];
	if (studyview) studyview->block_signals(true);
	imagesbox->listWidget->blockSignals(true);
	disconnect(imagesbox->listWidget,SIGNAL(itemSelectionChanged()),this,SLOT(update_selection()));
	disconnect(imagesbox->listWidget,SIGNAL(itemChanged(QListWidgetItem*)),this,SLOT(update_selection()));
	graphicswidget_m->clear_();
	graphicswidget_y->clear_();
	graphicswidget_x->clear_();
	histogramview->clear__();
	if (item__)
	{
		imagesbox->listWidget->removeItemWidget(item__);
		delete item__;
		item__ = nullptr;
	}
	imagesbox->listWidget->reset();
	remove_from_studyview(ivariant->id);
	scene3dimages.remove(ivariant->id);
	delete ivariant;
	ivariant = nullptr;
	update_selection();
	connect(imagesbox->listWidget,SIGNAL(itemSelectionChanged()),this,SLOT(update_selection()));
	connect(imagesbox->listWidget,SIGNAL(itemChanged(QListWidgetItem*)),this,SLOT(update_selection()));
	imagesbox->listWidget->blockSignals(false);
	if (studyview)
	{
		studyview->block_signals(false);
		if (scene3dimages.empty())
		{
			studyview->clear_();
			if (studyview->get_in_tab()) studyview->calculate_grid(2);
		}
	}
quit__:
	lock0 = false;
	if (ok3d) glwidget->set_skip_draw(false);
#ifdef ALIZA_PRINT_COUNT_GL_OBJ
	std::cout << "Num VBOs " << GLWidget::get_count_vbos() << std::endl;
#endif
}

void Aliza::delete_checked_images()
{
	delete_checked_unchecked(true);
}

void Aliza::delete_unchecked_images()
{
	delete_checked_unchecked(false);
}

ImageVariant * Aliza::get_image(int id)
{
	if (id < 0) return nullptr;
	if (scene3dimages.contains(id)) return scene3dimages[id];
	return nullptr;
}

const ImageVariant * Aliza::get_image(int id) const
{
	if (id < 0) return nullptr;
	if (scene3dimages.contains(id)) return scene3dimages.value(id);
	return nullptr;
}

int Aliza::get_selected_image_id()
{
	int id{-1};
	QList<QListWidgetItem*> l = imagesbox->listWidget->selectedItems();
	if (!l.empty())
	{
		const ListWidgetItem2 * i = static_cast<const ListWidgetItem2*>(l.at(0));
		if (i) id = i->get_id();
	}
	return id;
}

ImageVariant * Aliza::get_selected_image()
{
	QList<QListWidgetItem*> l = imagesbox->listWidget->selectedItems();
	if (!l.empty())
	{
		ListWidgetItem2 * i = static_cast<ListWidgetItem2*>(l.at(0));
		if (i) return i->get_image_from_item();
	}
	return nullptr;
}

const ImageVariant * Aliza::get_selected_image_const() const
{
	QList<QListWidgetItem*> l = imagesbox->listWidget->selectedItems();
	if (!l.empty())
	{
		const ListWidgetItem2 * i = static_cast<const ListWidgetItem2*>(l.at(0));
		if (i) return i->get_image_from_item_const();
	}
	return nullptr;
}

static long long count_elscint = 0;
static long long count_elscint_copy = 0;
static void process_elscint_dir(
	const QString & p,
	const QString & outp,
	QProgressDialog * pb)
{
	if (p.isEmpty()) return;
	QApplication::processEvents();
	if (pb && pb->wasCanceled()) return;
	QDir dir(p);
	QStringList dlist = dir.entryList(QDir::Dirs|QDir::NoDotAndDotDot);
	QStringList flist = dir.entryList(QDir::Files|QDir::Readable,QDir::Name);
	QStringList filenames;
	for (int x = 0; x < flist.size(); ++x)
	{
		QApplication::processEvents();
		if (pb && pb->wasCanceled()) return;
		const QString tmp0 = dir.absolutePath() + QString("/") + flist.at(x);
		if (DicomUtils::is_dicom_file(tmp0)) filenames.push_back(tmp0);
	}
	flist.clear();
	//
	for (int x = 0; x < filenames.size(); ++x)
	{
		QApplication::processEvents();
		if (pb && pb->wasCanceled()) return;
		QFileInfo fi(filenames.at(x));
		const QString tmp9 = outp + QString("/") + fi.fileName();
		try
		{
			if (DicomUtils::convert_elscint(filenames.at(x), tmp9))
			{
				++count_elscint;
			}
			else
			{
				if (QFile::copy(filenames.at(x), tmp9))
				{
					++count_elscint_copy;
				}
			}
		}
		catch (const mdcm::ParseException & pe)
		{
#ifdef ALIZA_VERBOSE
			std::cout << "mdcm::ParseException in process_elscint_dir:\n"
				<< pe.GetLastElement().GetTag() << std::endl;
#else
			(void)pe;
#endif
		}
		catch (const std::exception & ex)
		{
#ifdef ALIZA_VERBOSE
			std::cout << "Exception in process_elscint_dir:\n"
				<< ex.what() << std::endl;
#else
			(void)ex;
#endif
		}
	}
	filenames.clear();
	//
	for (int j = 0; j < dlist.size(); ++j)
	{
		QApplication::processEvents();
		if (pb && pb->wasCanceled()) return;
		QDir d(outp + QString("/") + dlist.at(j));
		if (!d.exists()) d.mkpath(d.absolutePath());
		process_elscint_dir(
			dir.absolutePath() + QString("/") + dlist.at(j),
			d.absolutePath(),
			pb);
	}
	dlist.clear();
}

bool Aliza::load_3d(
	ImageVariant * ivariant,
	bool force_no_filtering,
	bool skip_settings,
	bool skip_icon,
	bool skip_bb)
{
	if (!ivariant) return false;
	bool ok{};
	bool ok3d = check_3d();
	int max_3d_tex_size{};
	if (ok3d)
	{
		max_3d_tex_size = glwidget->max_3d_texture_size;
	}
	bool change_size{};
	unsigned int size_x_{};
	unsigned int size_y_{};
	if (!skip_settings && settingswidget->get_resize())
	{
		change_size = true;
		size_x_ = settingswidget->get_size_x();
		size_y_ = settingswidget->get_size_y();
	}
	if (force_no_filtering)
	{
		ivariant->di->filtering = 0;
	}
	else
	{
		if (!ivariant->equi)
		{
			ivariant->di->filtering = 0;
		}
		else
		{
			if (!skip_settings)
			{
				ivariant->di->filtering =
					settingswidget->get_filtering();
			}
		}
	}
	if (ivariant->image_type >= 0 && ivariant->image_type < 10)
	{
		ok = CommonUtils::reload_monochrome(
			ivariant,ok3d,(ok3d ? glwidget : nullptr), max_3d_tex_size,
			change_size, size_x_, size_y_);
	}
	else if (ivariant->image_type >= 10 && ivariant->image_type < 20)
	{
		ok = CommonUtils::reload_rgb_rgba(ivariant);
	}
	else if (ivariant->image_type >= 20 && ivariant->image_type < 30)
	{
		ok = CommonUtils::reload_rgb_rgba(ivariant);
	}
	else
	{
		return false;
	}
	if (ivariant->equi)
	{
		if (ivariant->di->idimz < 7)
			ivariant->di->transparency = false;
	}
	else
	{
		if (!ivariant->one_direction) ivariant->di->transparency = false;
		ivariant->di->filtering = 0;
	}
	if (!skip_icon) IconUtils::icon(ivariant);
	if (!skip_bb) CommonUtils::reset_bb(ivariant);
	if (ivariant && ivariant->di->opengl_ok &&
		!ivariant->di->skip_texture &&
		(ivariant->di->idimz != static_cast<int>(ivariant->di->image_slices.size())))
	{
#ifdef ALIZA_VERBOSE
		std::cout << "ivariant->di->idimz != ivariant->di->image_slices.size()" << std::endl;
#endif
		ivariant->di->close();
		ivariant->di->skip_texture = true;
	}
	return ok;
}

void Aliza::reload_3d(
	ImageVariant * ivariant,
	bool update,
	bool force_no_filtering,
	bool skip_settings,
	bool skip_icon,
	bool skip_bb)
{
	if (!ivariant) return;
	const bool ok =
		load_3d(
			ivariant,
			force_no_filtering,
			skip_settings,
			skip_icon,
			skip_bb);
	if (!ok) return;
	imagesbox->listWidget->reset();
	int r{-1};
	for (int x = 0; x < imagesbox->listWidget->count(); ++x)
	{
		ListWidgetItem2 * item = static_cast<ListWidgetItem2*>(imagesbox->listWidget->item(x));
		if (item && item->get_id() == ivariant->id)
		{
			r = x;
			item->setIcon(ivariant->icon);
			break;
		}
	}
	if (update && r > -1)
	{
		imagesbox->listWidget->setCurrentRow(r);
		update_selection();
	}
}

void Aliza::set_from_slice(int i)
{
	ImageVariant * v = get_selected_image();
	if (!v) return;
	v->di->from_slice = i;
	calculate_bb();
}

void Aliza::set_to_slice(int i)
{
	ImageVariant * v = get_selected_image();
	if (!v) return;
	v->di->to_slice = i;
	calculate_bb();
}

void Aliza::set_us_center(ImageVariant * v, double i)
{
	if (v)
	{
		v->di->us_window_center = i;
		const double div_ =
			((v->di->rmax - v->di->rmin) > 0)
			?
			v->di->rmax - v->di->rmin
			:
			1e-6;
		v->di->window_center = (i + (-v->di->rmin)) / div_;
		if (v->di->window_center < 0) v->di->window_center = 0.0;
		if (v->di->window_center > 1) v->di->window_center = 1.0;
	}
}

void Aliza::set_us_width(ImageVariant * v, double i)
{
	if (v)
	{
		v->di->us_window_width = i;
		const double div_ =
			((v->di->rmax - v->di->rmin) > 0)
			?
			v->di->rmax - v->di->rmin
			:
			1e-6;
		v->di->window_width = i / div_;
		if (v->di->window_width <= 0) v->di->window_width = 1e-6;
		if (v->di->window_width  > 1) v->di->window_width = 1.0;
	}
}

void Aliza::center_from_spinbox(double i)
{
	ImageVariant * v = get_selected_image();
	if (!v) return;
	toolbox2D->disconnect_sliders();
	set_us_center(v, i);
	if (v->group_id >= 0) update_group_center(v);
	toolbox2D->center_horizontalSlider->setValue(v->di->us_window_center);
#if 0
	if (!run__)
#endif
	{
		if (check_3d() && check_3d_visible()) glwidget->updateGL();
		if (check_2d_visible())
		{
			if (!graphicswidget_m->run__)
				graphicswidget_m->update_image(0, false);
			if (multiview)
			{
				graphicswidget_y->update_image(0, false);
				graphicswidget_x->update_image(0, false);
			}
		}
	}
	if (histogram_mode) histogramview->update_window(v);
	toolbox2D->connect_sliders();
}

void Aliza::width_from_spinbox(double i)
{
	ImageVariant * v = get_selected_image();
	if (!v) return;
	toolbox2D->disconnect_sliders();
	set_us_width(v, i);
	if (v->group_id >= 0) update_group_width(v);
	toolbox2D->width_horizontalSlider->setValue(v->di->us_window_width);
#if 0
	if (!run__)
#endif
	{
		if (check_3d() && check_3d_visible()) glwidget->updateGL();
		if (check_2d_visible())
		{
			if (!graphicswidget_m->run__)
				graphicswidget_m->update_image(0, false);
			if (multiview)
			{
				graphicswidget_y->update_image(0, false);
				graphicswidget_x->update_image(0, false);
			}
		}
	}
	if (histogram_mode) histogramview->update_window(v);
	toolbox2D->connect_sliders();
}

void Aliza::set_lut_function0(int x)
{
	ImageVariant * v = get_selected_image();
	if (!v) return;
	v->di->lut_function = x;
}

void Aliza::set_lut_function1(int x)
{
	const ImageVariant * v = get_selected_image_const();
	if (!v) return;
	set_lut_function0(x);
#if 0
	if (!run__)
#endif
	{
		if (check_3d() && check_3d_visible()) glwidget->updateGL();
		if (check_2d_visible())
		{
			if (!graphicswidget_m->run__)
				graphicswidget_m->update_image(0, false);
			if (multiview)
			{
				graphicswidget_y->update_image(0, false);
				graphicswidget_x->update_image(0, false);
			}
		}
	}
}

void Aliza::set_lut(int i)
{
	ImageVariant * v = get_selected_image();
	if (!v) return;
	v->di->selected_lut = static_cast<short>(i);
	if (v->group_id >= 0)
	{
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
		QMap<int, ImageVariant*>::const_iterator iv = scene3dimages.cbegin();
		while (iv != scene3dimages.cend())
#else
		QMap<int, ImageVariant*>::const_iterator iv = scene3dimages.constBegin();
		while (iv != scene3dimages.constEnd())
#endif
		{
			ImageVariant * v2 = iv.value();
			if (v2 && (v->group_id == v2->group_id))
			{
				v2->di->selected_lut = static_cast<short>(i);
			}
			++iv;
		}
	}
#if 0
	if (!run__)
#endif
	{
		if (check_3d() && check_3d_visible()) glwidget->updateGL();
		if (check_2d_visible())
		{
			if (!graphicswidget_m->run__)
				graphicswidget_m->update_image(0, false);
			if (multiview)
			{
				graphicswidget_y->update_image(0, false);
				graphicswidget_x->update_image(0, false);
			}
		}
	}
}

void Aliza::set_browser2(BrowserWidget2 * i)
{
	browser2 = i;
}

void Aliza::set_settingswidget(SettingsWidget * i)
{
	settingswidget = i;
}

void Aliza::set_imagesbox(ImagesBox * i)
{
	imagesbox = i;
}

const ImagesBox * Aliza::get_imagesbox() const
{
	return imagesbox;
}

void Aliza::set_toolbox(ToolBox * i)
{
	toolbox = i;
}

void Aliza::set_toolbox2D(ToolBox2D * i)
{
	toolbox2D = i;
}

void Aliza::set_glwidget(GLWidget * i)
{
	glwidget = i;
	if (glwidget) glwidget->set_selected_images_ptr(&selected_images);
}

void Aliza::set_graphicswidget_m(GraphicsWidget * i)
{
	graphicswidget_m = i;
}

void Aliza::set_graphicswidget_y(GraphicsWidget * i)
{
	graphicswidget_y = i;
}

void Aliza::set_graphicswidget_x(GraphicsWidget * i)
{
	graphicswidget_x = i;
}

void Aliza::set_sliderwidget_m(SliderWidget * i)
{
	slider_m = i;
}

void Aliza::set_sliderwidget_y(SliderWidget * i)
{
	slider_y = i;
}

void Aliza::set_sliderwidget_x(SliderWidget * i)
{
	slider_x = i;
}

void Aliza::set_zrangewidget(ZRangeWidget * i)
{
	zrangewidget = i;
}

void Aliza::set_histogramview(HistogramView * i)
{
	histogramview = i;
}

void Aliza::set_lutwidget2(LUTWidget * i)
{
	lutwidget2 = i;
}

void Aliza::set_trans3D_action(QAction * a)
{
	trans3DAct = a;
}

void Aliza::set_studyview(StudyViewWidget * s)
{
	studyview = s;
}

void Aliza::set_axis_actions(
	QAction * act_z,
	QAction * act_y,
	QAction * act_x,
	QAction * act_zyx,
	QAction * act_histogram)
{
	graphicsAct_Z = act_z;
	graphicsAct_Y = act_y;
	graphicsAct_X = act_x;
	zyxAct = act_zyx;
	histogramAct = act_histogram;
}

void Aliza::set_3D_views_actions(
	QAction * slicesAct_,
	QAction * zlockAct_,
	QAction * oneAct_,
	QAction * show3DAct_)
{
	slicesAct = slicesAct_;
	zlockAct = zlockAct_;
	oneAct = oneAct_;
	show3DAct = show3DAct_;
}

void Aliza::set_2D_views_actions(
	QAction * frames2DAct_,
	QAction * distanceAct_,
	QAction * rectAct_,
	QAction * segmentAct_,
	QAction * cursorAct_,
	QAction * collisionAct_,
	QAction * show2DAct_)
{
	frames2DAct = frames2DAct_;
	distanceAct = distanceAct_;
	rectAct = rectAct_;
	segmentAct = segmentAct_;
	cursorAct = cursorAct_;
	collisionAct = collisionAct_;
	show2DAct = show2DAct_;
}

void Aliza::set_anim3Dwidget(AnimWidget * i)
{
	anim3Dwidget = i;
}

void Aliza::set_anim2Dwidget(AnimWidget * i)
{
	anim2Dwidget = i;
}

void Aliza::update_toolbox(const ImageVariant * v)
{
	if (!v)
	{
		toolbox2D->setEnabled(false);
		return;
	}
	toolbox2D->setEnabled(true);
	disconnect_tools();
	trans3DAct->blockSignals(true);
	zlockAct->blockSignals(true);
	oneAct->blockSignals(true);
	toolbox2D->disconnect_sliders();
	const int selected_z_frame = v->di->selected_z_slice;
	if (check_3d())
	{
		slicesAct->setEnabled(true);
	}
	else
	{
		slicesAct->setEnabled(false);
	}
	toolbox2D->set_max_width(v->di->rmax - v->di->rmin + 1.0);
	toolbox2D->set_window_upper(v->di->rmax);
	toolbox2D->set_window_lower(v->di->rmin);
	toolbox2D->toggle_locked_values(v->di->lock_level2D);
	if (v->image_type >= 0 && v->image_type < 10)
	{
		if (v->frame_levels.contains(selected_z_frame))
		{
			const FrameLevel & fl = v->frame_levels.value(selected_z_frame);
			toolbox2D->set_locked_center(fl.us_window_center);
			toolbox2D->set_locked_width(fl.us_window_width);
		}
		else
		{
			toolbox2D->set_locked_center(v->di->default_us_window_center);
			toolbox2D->set_locked_width(v->di->default_us_window_width);
		}
	}
	else
	{
		toolbox2D->set_locked_center(0.0);
		toolbox2D->set_locked_width(0.0);
	}
	toolbox2D->set_width(v->di->us_window_width);
	toolbox2D->set_center(v->di->us_window_center);
	if (v->image_type == 0 || v->image_type == 1)
	{
		toolbox2D->set_maxwindow(v->di->maxwindow);
		toolbox2D->enable_maxwindow(true);
	}
	else if (v->image_type == 4)
	{
		toolbox2D->set_maxwindow(true);
		toolbox2D->enable_maxwindow(false);
	}
	else
	{
		toolbox2D->set_maxwindow(false);
		toolbox2D->enable_maxwindow(false);
	}
	if (v->di->disable_int_level)
	{
		toolbox2D->center_horizontalSlider->setEnabled(false);
		toolbox2D->width_horizontalSlider->setEnabled(false);
		toolbox2D->center_horizontalSlider->setValue(0);
		toolbox2D->width_horizontalSlider->setValue(0);
	}
	else
	{
		toolbox2D->center_horizontalSlider->setEnabled(true);
		toolbox2D->width_horizontalSlider->setEnabled(true);
		toolbox2D->center_horizontalSlider->setValue(static_cast<int>(v->di->us_window_center));
		toolbox2D->width_horizontalSlider->setValue(static_cast<int>(v->di->us_window_width));
	}
	//
	toolbox2D->set_lut_function(v->di->lut_function);
	set_lut_function0(v->di->lut_function);
	//
	const int tmpx = ((v->di->idimx - 1) < 0) ? 0 : (v->di->idimx - 1);
	const int tmpy = ((v->di->idimy - 1) < 0) ? 0 : (v->di->idimy - 1);
	const int tmpz = ((v->di->idimz - 1) < 0) ? 0 : (v->di->idimz - 1);
	if (v->di->lock_2Dview)
	{
		zlockAct->setChecked(true);
		zlockAct->setIcon(anchor_icon);
		oneAct->setEnabled(true);
		v->di->from_slice = selected_z_frame;
		if (v->di->lock_single)
		{
			v->di->to_slice = selected_z_frame;
		}
		else
		{
			v->di->to_slice = v->di->idimz > 1 ? v->di->idimz - 1 : 0;
		}
	}
	else
	{
		zlockAct->setChecked(false);
		zlockAct->setIcon(anchor2_icon);
		oneAct->setEnabled(false);
	}
	oneAct->setChecked(v->di->lock_single);
	zrangewidget->set_spanslider_max(tmpz);
	zrangewidget->set_span(v->di->from_slice, v->di->to_slice);
	zrangewidget->setEnabled(!v->di->lock_2Dview);
	switch (graphicswidget_m->get_axis())
	{
	case 0:
		{
			slider_m->set_slider_max(tmpx);
			slider_m->set_slice(v->di->selected_x_slice);
		}
		break;
	case 1:
		{
			slider_m->set_slider_max(tmpy);
			slider_m->set_slice(v->di->selected_y_slice);
		}
		break;
	case 2:
		{
			slider_m->set_slider_max(tmpz);
			slider_m->set_slice(selected_z_frame);
		}
		break;
	default:
		break;
	}
	slider_y->set_slider_max(tmpy);
	slider_y->set_slice(v->di->selected_y_slice);
	slider_x->set_slider_max(tmpx);
	slider_x->set_slice(v->di->selected_x_slice);
	lutwidget2->set_lut(v->di->selected_lut);
	if (v->di->transparency)
	{
		trans3DAct->setChecked(true);
		trans3DAct->setIcon(trans_icon);
		toolbox->alpha_label->setEnabled(true);
		toolbox->alpha_doubleSpinBox->show();
	}
	else
	{
		trans3DAct->setChecked(false);
		trans3DAct->setIcon(notrans_icon);
		toolbox->alpha_label->setEnabled(false);
		toolbox->alpha_doubleSpinBox->hide();
	}
	if (!v->frame_times.empty())
	{
		anim2Dwidget->frametime_spinBox->hide();
	}
	else
	{
		anim2Dwidget->frametime_spinBox->show();
	}
	connect_tools();
	trans3DAct->blockSignals(false);
	zlockAct->blockSignals(false);
	oneAct->blockSignals(false);
	toolbox2D->connect_sliders();
}

void Aliza::connect_slots()
{
	connect(imagesbox->listWidget,           SIGNAL(itemSelectionChanged()),        this, SLOT(update_selection()));
	connect(imagesbox->listWidget,           SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(update_selection()));
	connect(imagesbox->actionClear,          SIGNAL(triggered()), this, SLOT(delete_image()));
	connect(imagesbox->actionClearChecked,   SIGNAL(triggered()), this, SLOT(delete_checked_images()));
	connect(imagesbox->actionClearUnChek,    SIGNAL(triggered()), this, SLOT(delete_unchecked_images()));
	connect(imagesbox->actionClearAll,       SIGNAL(triggered()), this, SLOT(clear_ram()));
	connect(imagesbox->actionCheck,          SIGNAL(triggered()), this, SLOT(trigger_check_all()));
	connect(imagesbox->actionUncheck,        SIGNAL(triggered()), this, SLOT(trigger_uncheck_all()));
	connect(imagesbox->actionColor,          SIGNAL(triggered()), this, SLOT(trigger_image_color()));
	connect(imagesbox->actionReloadHistogram,SIGNAL(triggered()), this, SLOT(trigger_reload_histogram()));
	connect(imagesbox->actionROIInfo,        SIGNAL(triggered()), this, SLOT(trigger_show_roi_info()));
	//
	connect(imagesbox->contours_tableWidget, SIGNAL(itemChanged(QTableWidgetItem*)), this, SLOT(update_visible_rois(QTableWidgetItem*)));
	//
#if 0
	connect(imagesbox->actionTmp,            SIGNAL(triggered()), this, SLOT(trigger_tmp()));
#endif
	//
	//
	if (glwidget)
	{
		connect(toolbox->ortho_radioButton,      SIGNAL(toggled(bool)),        glwidget, SLOT(set_ortho(bool)));
		connect(toolbox->fov_doubleSpinBox,      SIGNAL(valueChanged(double)), glwidget, SLOT(set_fov(double)));
		connect(toolbox->far_doubleSpinBox,      SIGNAL(valueChanged(double)), glwidget, SLOT(set_far(double)));
		connect(toolbox->alpha_doubleSpinBox,    SIGNAL(valueChanged(double)), glwidget, SLOT(set_alpha(double)));
		connect(toolbox->bright_doubleSpinBox,   SIGNAL(valueChanged(double)), glwidget, SLOT(set_brightness(double)));
		connect(toolbox->contours_checkBox,      SIGNAL(toggled(bool)),        glwidget, SLOT(set_display_contours(bool)));
		connect(toolbox->cube_checkBox,          SIGNAL(toggled(bool)),        glwidget, SLOT(set_cube(bool)));
		connect(settingswidget->adjust_checkBox, SIGNAL(toggled(bool)),        this, SLOT(set_adjust_glwidget(bool)));
		connect(settingswidget->adjust_spinBox,  SIGNAL(valueChanged(int)),    this, SLOT(set_adjust_value_glwidget(int)));
	}
	connect(anim3Dwidget->frametime_spinBox,  SIGNAL(valueChanged(int)), this,             SLOT(set_frametime_3D(int)));
	connect(anim3Dwidget->group_pushButton,   SIGNAL(clicked()),         this,             SLOT(create_group()));
	connect(anim3Dwidget->remove_pushButton,  SIGNAL(clicked()),         this,             SLOT(remove_group()));
	connect(anim2Dwidget->frametime_spinBox,  SIGNAL(valueChanged(int)), graphicswidget_m, SLOT(set_frametime_2D(int)));
	connect(settingswidget->time_s__checkBox, SIGNAL(toggled(bool)),     graphicswidget_m, SLOT(set_frame_time_unit(bool)));
	connect(graphicswidget_m->graphicsview,   SIGNAL(bb_changed()), this, SLOT(calculate_bb()));
	connect(toolbox2D->resetlevel_pushButton, SIGNAL(clicked()), this, SLOT(reset_level()));
	connect(studyview, SIGNAL(update_scouts_required()), this, SLOT(update_studyview_intersections()));
	//
	anim3D_timer->setSingleShot(true);
	connect(anim3D_timer, SIGNAL(timeout()), this, SLOT(animate_()));
	//
	connect_tools();
}

void Aliza::connect_tools()
{
	connect(toolbox2D->width_doubleSpinBox,  SIGNAL(valueChanged(double)),     this, SLOT(width_from_spinbox(double)));
	connect(toolbox2D->center_doubleSpinBox, SIGNAL(valueChanged(double)),     this, SLOT(center_from_spinbox(double)));
	connect(toolbox2D->maxwin_pushButton,    SIGNAL(toggled(bool)),            this, SLOT(toggle_maxwindow(bool)));
	connect(toolbox2D->comboBox,             SIGNAL(currentIndexChanged(int)), this, SLOT(set_lut_function1(int)));
	connect(toolbox2D->lock_pushButton,      SIGNAL(toggled(bool)),            this, SLOT(toggle_lock_window(bool)));
	if (!graphicswidget_m->run__)
	{
		connect(slider_m->slices_slider, SIGNAL(valueChanged(int)), this, SLOT(set_selected_slice2D_m(int)));
	}
	connect(slider_y->slices_slider, SIGNAL(valueChanged(int)), this, SLOT(set_selected_slice2D_y(int)));
	connect(slider_x->slices_slider, SIGNAL(valueChanged(int)), this, SLOT(set_selected_slice2D_x(int)));
	connect(zrangewidget->spanslider, SIGNAL(lowerValueChanged(int)), this, SLOT(set_from_slice(int)));
	connect(zrangewidget->spanslider, SIGNAL(upperValueChanged(int)), this, SLOT(set_to_slice(int)));
	connect(lutwidget2->comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(set_lut(int)));
	connect(trans3DAct, SIGNAL(toggled(bool)), this, SLOT(set_transparency(bool)));
}

void Aliza::disconnect_tools()
{
	disconnect(toolbox2D->width_doubleSpinBox,  SIGNAL(valueChanged(double)),     this, SLOT(width_from_spinbox(double)));
	disconnect(toolbox2D->center_doubleSpinBox, SIGNAL(valueChanged(double)),     this, SLOT(center_from_spinbox(double)));
	disconnect(toolbox2D->maxwin_pushButton,    SIGNAL(toggled(bool)),            this, SLOT(toggle_maxwindow(bool)));
	disconnect(toolbox2D->comboBox,             SIGNAL(currentIndexChanged(int)), this, SLOT(set_lut_function1(int)));
	disconnect(toolbox2D->lock_pushButton,      SIGNAL(toggled(bool)),            this, SLOT(toggle_lock_window(bool)));
	if (!graphicswidget_m->run__)
	{
		disconnect(slider_m->slices_slider, SIGNAL(valueChanged(int)), this, SLOT(set_selected_slice2D_m(int)));
	}
	disconnect(slider_y->slices_slider, SIGNAL(valueChanged(int)), this, SLOT(set_selected_slice2D_y(int)));
	disconnect(slider_x->slices_slider, SIGNAL(valueChanged(int)), this, SLOT(set_selected_slice2D_x(int)));
	disconnect(zrangewidget->spanslider, SIGNAL(lowerValueChanged(int)), this, SLOT(set_from_slice(int)));
	disconnect(zrangewidget->spanslider, SIGNAL(upperValueChanged(int)), this, SLOT(set_to_slice(int)));
	disconnect(lutwidget2->comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(set_lut(int)));
	disconnect(trans3DAct, SIGNAL(toggled(bool)), this, SLOT(set_transparency(bool)));
}

void Aliza::set_selected_slice2D_m(int j)
{
	ImageVariant * v = get_selected_image();
	if (v)
	{
		const int a = graphicswidget_m->get_axis();
		switch (a)
		{
		case 0: v->di->selected_x_slice = j;
			break;
		case 1: v->di->selected_y_slice = j;
			break;
		case 2: v->di->selected_z_slice = j;
			break;
		default:
			break;
		}
		if (v->group_id >= 0)
		{
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
			QMap<int, ImageVariant*>::const_iterator iv = scene3dimages.cbegin();
			while (iv != scene3dimages.cend())
#else
			QMap<int, ImageVariant*>::const_iterator iv = scene3dimages.constBegin();
			while (iv != scene3dimages.constEnd())
#endif
			{
				ImageVariant * v2 = iv.value();
				if (v2 && (v->group_id == v2->group_id))
				{
					switch (a)
					{
					case 0: v2->di->selected_x_slice = j;
						break;
					case 1: v2->di->selected_y_slice = j;
						break;
					case 2:
						{
							v2->di->selected_z_slice = j;
							if (v2->di->lock_2Dview)
							{
								if (v2->di->lock_single)
								{
									v2->di->from_slice = j;
									v2->di->to_slice   = j;
								}
								else
								{
									const int tmp0 = (v2->di->idimz > 1)
										? v2->di->idimz - 1
										: 0;
									v2->di->from_slice = j;
									v2->di->to_slice   = tmp0;
								}
							}
						}
						break;
					default:
						break;
					}
				}
				++iv;
			}
		}
		//
		{
			bool frame_level_available{};
			if (a == 2)
			{
				if (v->image_type >= 0 && v->image_type < 10)
				{
					if (v->frame_levels.contains(v->di->selected_z_slice))
					{
						const FrameLevel & fl = v->frame_levels.value(v->di->selected_z_slice);
						toolbox2D->set_locked_center(fl.us_window_center);
						toolbox2D->set_locked_width(fl.us_window_width);
						frame_level_available = true;
					}
					else
					{
						toolbox2D->set_locked_center(v->di->default_us_window_center);
						toolbox2D->set_locked_width(v->di->default_us_window_width);
					}
				}
				else
				{
					toolbox2D->set_locked_center(0.0);
					toolbox2D->set_locked_width(0.0);
				}
			}
			graphicswidget_m->set_slice_2D(v, 0, true, frame_level_available);
			if (!run__)
			{
				check_slice_collisions(
					const_cast<const ImageVariant *>(v),
					graphicswidget_m);
			}
			if (a == 2 && v->di->lock_2Dview)
			{
				zrangewidget->spanslider->blockSignals(true);
				if (v->di->lock_single)
				{
					v->di->from_slice = j;
					v->di->to_slice   = j;
					zrangewidget->set_span(j, j);
				}
				else
				{
					const int tmp0 = v->di->idimz > 1 ? v->di->idimz - 1 : 0;
					v->di->from_slice = j;
					v->di->to_slice   = tmp0;
					zrangewidget->set_span(j, tmp0);
				}
				if (check_3d() && check_3d_visible()) glwidget->updateGL();
				zrangewidget->spanslider->blockSignals(false);
			}
			if (multiview)
			{
				graphicswidget_y->update_frames();
				graphicswidget_x->update_frames();
				graphicswidget_y->update_selection_item();
				graphicswidget_x->update_selection_item();
			}
		}
	}
}

void Aliza::set_selected_slice2D_y(int j)
{
	ImageVariant * v = get_selected_image();
	if (!v) return;
	v->di->selected_y_slice = j;
	if (v->group_id >= 0)
	{
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
		QMap<int, ImageVariant*>::const_iterator iv = scene3dimages.cbegin();
		while (iv != scene3dimages.cend())
#else
		QMap<int, ImageVariant*>::const_iterator iv = scene3dimages.constBegin();
		while (iv != scene3dimages.constEnd())
#endif
		{
			ImageVariant * v2 = iv.value();
			if (v2 && (v->group_id == v2->group_id))
			{
				v2->di->selected_y_slice = j;
			}
			++iv;
		}
	}
	graphicswidget_y->set_slice_2D(v, 0, false);
	graphicswidget_m->update_frames();
	graphicswidget_x->update_frames();
}

void Aliza::set_selected_slice2D_x(int j)
{
	ImageVariant * v = get_selected_image();
	if (!v) return;
	v->di->selected_x_slice = j;
	if (v->group_id >= 0)
	{
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
		QMap<int, ImageVariant*>::const_iterator iv = scene3dimages.cbegin();
		while (iv != scene3dimages.cend())
#else
		QMap<int, ImageVariant*>::const_iterator iv = scene3dimages.constBegin();
		while (iv != scene3dimages.constEnd())
#endif
		{
			ImageVariant * v2 = iv.value();
			if (v2 && (v->group_id == v2->group_id))
			{
				v2->di->selected_x_slice = j;
			}
			++iv;
		}
	}
	graphicswidget_x->set_slice_2D(v, 0, false);
	graphicswidget_m->update_frames();
	graphicswidget_y->update_frames();
}

bool Aliza::check_3d()
{
	if (glwidget && glwidget->opengl_init_done && !glwidget->no_opengl3)
	{
		return true;
	}
	return false;
}

bool Aliza::check_3d_visible()
{
	if (show3DAct && show3DAct->isChecked())
	{
		return true;
	}
	return false;
}

bool Aliza::check_2d_visible()
{
	if (show2DAct && show2DAct->isChecked())
	{
		return true;
	}
	return false;
}

void Aliza::set_axis_2D(int a, bool rect_mode)
{
	multiview = false;
	histogram_mode = false;
	graphicswidget_m->set_multiview(multiview);
	ImageVariant * v = get_selected_image();
	if (!v) return;
	graphicswidget_m->set_axis(a);
	if (rect_mode)
	{
		if (graphicswidget_m->get_axis() == 2)
		{
			graphicswidget_m->graphicsview->handle_rect->show();
			graphicswidget_m->graphicsview->selection_item->hide();
		}
		else
		{
			graphicswidget_m->graphicsview->handle_rect->hide();
			graphicswidget_m->graphicsview->selection_item->show();
		}
		graphicswidget_x->graphicsview->selection_item->show();
		graphicswidget_y->graphicsview->selection_item->show();
	}
	else
	{
		graphicswidget_m->graphicsview->handle_rect->hide();
		graphicswidget_m->graphicsview->selection_item->hide();
		graphicswidget_x->graphicsview->selection_item->hide();
		graphicswidget_y->graphicsview->selection_item->hide();
	}
	update_toolbox(v);
	if (a == 2)
	{
		graphicswidget_m->set_slice_2D(v, 1, true);
		check_slice_collisions(const_cast<const ImageVariant *>(v), graphicswidget_m);
	}
	else
	{
		graphicswidget_m->set_slice_2D(v, 1, false);
	}
	histogramview->clear__();
}

void Aliza::set_histogram()
{
	multiview = false;
	histogram_mode = true;
	graphicswidget_m->set_multiview(multiview);
	const ImageVariant * v = get_selected_image_const();
	if (v) histogramview->update__(v);
}

void Aliza::set_axis_zyx(bool rect_mode)
{
	multiview = true;
	histogram_mode = true;
	graphicswidget_m->set_multiview(multiview);
	ImageVariant * v = get_selected_image();
	if (!v) return;
	graphicswidget_m->set_axis(2);
	if (rect_mode)
	{
		graphicswidget_m->graphicsview->handle_rect->show();
		graphicswidget_x->graphicsview->selection_item->show();
		graphicswidget_y->graphicsview->selection_item->show();
		graphicswidget_m->graphicsview->selection_item->hide();
	}
	else
	{
		graphicswidget_m->graphicsview->handle_rect->hide();
		graphicswidget_m->graphicsview->selection_item->hide();
		graphicswidget_x->graphicsview->selection_item->hide();
		graphicswidget_y->graphicsview->selection_item->hide();
	}
	update_toolbox(v);
	graphicswidget_m->set_slice_2D(v, 1, true);
	check_slice_collisions(const_cast<const ImageVariant *>(v), graphicswidget_m);
	graphicswidget_y->set_slice_2D(v, 1, false);
	graphicswidget_x->set_slice_2D(v, 1, false);
	histogramview->update__(v);
}

void Aliza::toggle_rect(bool t)
{
	rect_selection = t;
	const bool ok3d = check_3d();
	if (ok3d) glwidget->rect_selection = rect_selection;
	graphicswidget_m->set_bb(t);
	graphicswidget_y->set_bb(t);
	graphicswidget_x->set_bb(t);
	if (t)
	{
		if (graphicswidget_m->get_axis() == 2)
		{
			graphicswidget_m->graphicsview->handle_rect->show();
			graphicswidget_m->graphicsview->selection_item->hide();
		}
		else
		{
			graphicswidget_m->graphicsview->handle_rect->hide();
			graphicswidget_m->graphicsview->selection_item->show();
		}
		graphicswidget_x->graphicsview->selection_item->show();
		graphicswidget_y->graphicsview->selection_item->show();
	}
	else
	{
		graphicswidget_m->graphicsview->handle_rect->hide();
		graphicswidget_m->graphicsview->selection_item->hide();
		graphicswidget_x->graphicsview->selection_item->hide();
		graphicswidget_y->graphicsview->selection_item->hide();
	}
	calculate_bb();
}

void Aliza::calculate_bb()
{
	ImageVariant * v = get_selected_image();
	if (!v) return;
	if (rect_selection)
	{
		const double line_width = graphicswidget_m->graphicsview->handle_rect->get_width();
		const QRectF rect = graphicswidget_m->graphicsview->handle_rect->boundingRect();
		const QPointF tmpp =
			graphicswidget_m->graphicsview->handle_rect->pos() +
			rect.topLeft() +
			QPointF(0.5 * line_width, 0.5 * line_width);
		double x_min{};
		double x_max{1.0};
		double y_min{};
		double y_max{1.0};
		double size[2];
		size[0] = (rect.width()  -line_width) > 1.0 ? rect.width()  - line_width : 1.0;
		size[1] = (rect.height() -line_width) > 1.0 ? rect.height() - line_width : 1.0;
		const int tmpp_x = static_cast<int>(round(tmpp.x()));
		const int tmpp_y = static_cast<int>(round(tmpp.y()));
		const int size_0 = static_cast<int>(round(size[0]));
		const int size_1 = static_cast<int>(round(size[1]));
		x_min =  static_cast<double>(tmpp_x) / static_cast<double>(v->di->idimx);
		x_max = (static_cast<double>(tmpp_x + size_0)) / static_cast<double>(v->di->idimx);
		y_min =  static_cast<double>(tmpp_y) / static_cast<double>(v->di->idimy);
		y_max = (static_cast<double>(tmpp_y + size_1)) / static_cast<double>(v->di->idimy);
		if (x_min < 0.0) x_min = 0.0;
		if (x_min > 1.0) x_min = 1.0;
		if (y_min < 0.0) y_min = 0.0;
		if (y_min > 1.0) y_min = 1.0;
		if (x_max < 0.0) x_max = 0.0;
		if (x_max > 1.0) x_max = 1.0;
		if (y_max < 0.0) y_max = 0.0;
		if (y_max > 1.0) y_max = 1.0;
		v->di->bb_x_min = x_min;
		v->di->bb_x_max = x_max;
		v->di->bb_y_min = y_min;
		v->di->bb_y_max = y_max;
		v->di->irect_index[0] = tmpp_x;
		v->di->irect_index[1] = tmpp_y;
		v->di->irect_size[0] = size_0;
		v->di->irect_size[1] = size_1;
		const QString info_text =
			QString("[") +
			QVariant(v->di->irect_index[0]).toString() +
			QString(",") +
			QVariant(v->di->irect_index[1]).toString() +
			QString("] ") +
			QVariant(v->di->irect_size[0]).toString() +
			QString("x") +
			QVariant(v->di->irect_size[1]).toString();
		graphicswidget_m->set_info_line_text(info_text);
		if (v->group_id >= 0)
		{
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
			QMap<int, ImageVariant*>::const_iterator iv = scene3dimages.cbegin();
			while (iv != scene3dimages.cend())
#else
			QMap<int, ImageVariant*>::const_iterator iv = scene3dimages.constBegin();
			while (iv != scene3dimages.constEnd())
#endif
			{
				ImageVariant * v2 = iv.value();
				if (v2 && (v->group_id == v2->group_id))
				{
					v2->di->from_slice = v->di->from_slice;
					v2->di->to_slice = v->di->to_slice;
					v2->di->bb_x_min = x_min; v2->di->bb_x_max = x_max;
					v2->di->bb_y_min = y_min; v2->di->bb_y_max = y_max;
					v2->di->irect_index[0] = tmpp_x;
					v2->di->irect_index[1] = tmpp_y;
					v2->di->irect_size[0]  = size_0;
					v2->di->irect_size[1]  = size_1;
				}
				++iv;
			}
		}
	}
	else
	{
		if (v->group_id >= 0)
		{
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
			QMap<int, ImageVariant*>::const_iterator iv = scene3dimages.cbegin();
			while (iv != scene3dimages.cend())
#else
			QMap<int, ImageVariant*>::const_iterator iv = scene3dimages.constBegin();
			while (iv != scene3dimages.constEnd())
#endif
			{
				ImageVariant * v2 = iv.value();
				if (v2 && (v->group_id == v2->group_id))
				{
					v2->di->from_slice = v->di->from_slice;
					v2->di->to_slice   = v->di->to_slice;
				}
				++iv;
			}
		}
		graphicswidget_m->set_info_line_text(QString(""));
	}
	if (selected_images.size() > 1)
	{
		int j{};
		double tmpx{};
		double tmpy{};
		double tmpz{};
		for (int x = 0; x < selected_images.size(); ++x)
		{
			ImageVariant * tmpv = selected_images.at(x);
			if (tmpv)
			{
				if (tmpv->equi) update_center(tmpv);
				++j;
				tmpx += static_cast<double>(tmpv->di->center_x);
				tmpy += static_cast<double>(tmpv->di->center_y);
				tmpz += static_cast<double>(tmpv->di->center_z);
			}
		}
		if (j > 0)
		{
			v->di->center_x = tmpx/static_cast<double>(j);
			v->di->center_y = tmpy/static_cast<double>(j);
			v->di->center_z = tmpz/static_cast<double>(j);
		}
	}
	else
	{
		if (v->equi)
		{
			update_center(v);
			if (v->group_id >= 0)
			{
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
				QMap<int, ImageVariant*>::const_iterator iv = scene3dimages.cbegin();
				while (iv != scene3dimages.cend())
#else
				QMap<int, ImageVariant*>::const_iterator iv = scene3dimages.constBegin();
				while (iv != scene3dimages.constEnd())
#endif
				{
					ImageVariant * v2 = iv.value();
					if (v2 && (v->group_id == v2->group_id))
					{
						update_center(v2);
					}
					++iv;
				}
			}
		}
		else
		{
			v->di->center_x = v->di->default_center_x;
			v->di->center_y = v->di->default_center_y;
			v->di->center_z = v->di->default_center_z;
		}
	}
#if 0
	if (!run__)
#endif
	{
		if ((2 != graphicswidget_m->get_axis()) && !graphicswidget_m->run__)
			graphicswidget_m->update_selection_item();
		if (multiview)
		{
			graphicswidget_y->update_selection_item();
			graphicswidget_x->update_selection_item();
		}
		if (check_3d() && check_3d_visible()) glwidget->updateGL();
	}
}

void Aliza::update_center(ImageVariant * v)
{
	if (!v) return;
	if (v->image_type >= 10) return;
	itk::Point<float, 3> p;
	int center_idx[3];
	int tmp0 = rect_selection
		? v->di->irect_index[0] + v->di->irect_size[0] / 2
		: v->di->idimx / 2;
	int tmp1 = rect_selection
		? v->di->irect_index[1] + v->di->irect_size[1] / 2
		: v->di->idimy / 2;
	int tmp2 = v->di->from_slice + (v->di->to_slice-v->di->from_slice) / 2;
	if (tmp0 < 0) tmp0 = 0;
	if (tmp0 > (v->di->idimx-1)) tmp0 = v->di->idimx - 1;
	if (tmp1 < 0) tmp1 = 0;
	if (tmp1 > (v->di->idimy-1)) tmp1 = v->di->idimy - 1;
	if (tmp2 < 0) tmp2 = 0;
	if (tmp2 > (v->di->idimz-1)) tmp2 = v->di->idimz - 1;
	center_idx[0] = tmp0;
	center_idx[1] = tmp1;
	center_idx[2] = tmp2;
	switch (v->image_type)
	{
	case 0:
		{
			if (v->pSS.IsNull()) return;
			ImageTypeSS::IndexType idx;
			idx[0] = center_idx[0];
			idx[1] = center_idx[1];
			idx[2] = center_idx[2];
			v->pSS->TransformIndexToPhysicalPoint(idx, p);
		}
		break;
	case 1:
		{
			if (v->pUS.IsNull()) return;
			ImageTypeUS::IndexType idx;
			idx[0] = center_idx[0];
			idx[1] = center_idx[1];
			idx[2] = center_idx[2];
			v->pUS->TransformIndexToPhysicalPoint(idx, p);
		}
		break;
	case 2:
		{
			if (v->pSI.IsNull()) return;
			ImageTypeSI::IndexType idx;
			idx[0] = center_idx[0];
			idx[1] = center_idx[1];
			idx[2] = center_idx[2];
			v->pSI->TransformIndexToPhysicalPoint(idx, p);
		}
		break;
	case 3:
		{
			if (v->pUI.IsNull()) return;
			ImageTypeUI::IndexType idx;
			idx[0] = center_idx[0];
			idx[1] = center_idx[1];
			idx[2] = center_idx[2];
			v->pUI->TransformIndexToPhysicalPoint(idx, p);
		}
		break;
	case 4:
		{
			if (v->pUC.IsNull()) return;
			ImageTypeUC::IndexType idx;
			idx[0] = center_idx[0];
			idx[1] = center_idx[1];
			idx[2] = center_idx[2];
			v->pUC->TransformIndexToPhysicalPoint(idx, p);
		}
		break;
	case 5:
		{
			if (v->pF.IsNull()) return;
			ImageTypeF::IndexType idx;
			idx[0] = center_idx[0];
			idx[1] = center_idx[1];
			idx[2] = center_idx[2];
			v->pF->TransformIndexToPhysicalPoint(idx, p);
		}
		break;
	case 6:
		{
			if (v->pD.IsNull()) return;
			ImageTypeD::IndexType idx;
			idx[0] = center_idx[0];
			idx[1] = center_idx[1];
			idx[2] = center_idx[2];
			v->pD->TransformIndexToPhysicalPoint(idx, p);
		}
		break;
	case 7:
		{
			if (v->pSLL.IsNull()) return;
			ImageTypeSLL::IndexType idx;
			idx[0] = center_idx[0];
			idx[1] = center_idx[1];
			idx[2] = center_idx[2];
			v->pSLL->TransformIndexToPhysicalPoint(idx, p);
		}
		break;
	case 8:
		{
			if (v->pULL.IsNull()) return;
			ImageTypeULL::IndexType idx;
			idx[0] = center_idx[0];
			idx[1] = center_idx[1];
			idx[2] = center_idx[2];
			v->pULL->TransformIndexToPhysicalPoint(idx, p);
		}
		break;
	default:
		return;
	}
	v->di->center_x = p[0];
	v->di->center_y = p[1];
	v->di->center_z = p[2];
}

void Aliza::set_view2d_mouse_modus(short m)
{
	graphicswidget_m->set_mouse_modus(m, true);
	graphicswidget_y->set_mouse_modus(m, false);
	graphicswidget_x->set_mouse_modus(m, false);
#if 0
	if (!run__)
#endif
	{
		if (!graphicswidget_m->run__)
		{
			graphicswidget_m->update_image(0, true);
			graphicswidget_m->update_frames();
		}
		if (multiview)
		{
			graphicswidget_y->update_frames();
			graphicswidget_x->update_frames();
		}
	}
}

void Aliza::set_show_frames_3d(bool t)
{
	if (check_3d())
	{
		glwidget->set_draw_frames_3d(t);
		if (
#if 0
			!run__ &&
#endif
			check_3d_visible() && glwidget->view == 0)
		{
			glwidget->updateGL();
		}
	}
}

void Aliza::set_transparency(bool t)
{
	ImageVariant * v = get_selected_image();
	if (v)
	{
		v->di->transparency = t;
		if (v->group_id >= 0)
		{
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
			QMap<int, ImageVariant*>::const_iterator iv = scene3dimages.cbegin();
			while (iv != scene3dimages.cend())
#else
			QMap<int, ImageVariant*>::const_iterator iv = scene3dimages.constBegin();
			while (iv != scene3dimages.constEnd())
#endif
			{
				ImageVariant * v2 = iv.value();
				if (v2 && (v->group_id == v2->group_id))
				{
					v2->di->transparency = t;
				}
				++iv;
			}
		}
		if (
#if 0
			!run__ &&
#endif
			check_3d()) glwidget->updateGL();
	}
	if (t)
	{
		trans3DAct->setIcon(trans_icon);
		toolbox->alpha_label->setEnabled(true);
		toolbox->alpha_doubleSpinBox->show();
	}
	else
	{
		trans3DAct->setIcon(notrans_icon);
		toolbox->alpha_label->setEnabled(false);
		toolbox->alpha_doubleSpinBox->hide();
	}
}

void Aliza::update_selection()
{
	selected_images.clear();
	QList<QListWidgetItem*> l = imagesbox->listWidget->selectedItems();
	QListWidgetItem * s = (l.size() > 0) ? l.at(0) : nullptr;
	if (l.size() == 1) // single selection mode
	{
		ListWidgetItem2 * i = static_cast<ListWidgetItem2*>(s);
		ImageVariant * v = (i) ? i->get_image_from_item() : nullptr;
		if (v)
		{
			graphicswidget_m->set_slice_2D(v, 0, true);
			if (multiview) graphicswidget_y->set_slice_2D(v, 0, false);
			if (multiview) graphicswidget_x->set_slice_2D(v, 0, false);
			update_selection_common1(v);
		}
	}
	update_selection_common2(s);
}

void Aliza::update_selection2()
{
	selected_images.clear();
	QList<QListWidgetItem*> l = imagesbox->listWidget->selectedItems();
	QListWidgetItem * s = (!l.empty()) ? l.at(0) : nullptr;
	if (l.size() == 1) // single selection mode
	{
		ListWidgetItem2 * i = static_cast<ListWidgetItem2*>(s);
		ImageVariant * v = (i) ? i->get_image_from_item() : nullptr;
		if (v)
		{
			graphicswidget_m->graphicsview->global_flip_x = false;
			graphicswidget_m->graphicsview->global_flip_y = false;
			graphicswidget_x->graphicsview->global_flip_x = false;
			graphicswidget_x->graphicsview->global_flip_y = false;
			graphicswidget_y->graphicsview->global_flip_x = false;
			graphicswidget_y->graphicsview->global_flip_y = false;
			graphicswidget_m->set_slice_2D(v, 1, true);
			if (multiview) graphicswidget_y->set_slice_2D(v, 1, false);
			if (multiview) graphicswidget_x->set_slice_2D(v, 1, false);
			update_selection_common1(v);
		}
	}
	update_selection_common2(s);
}

void Aliza::update_selection_common1(ImageVariant * v)
{
	if (histogram_mode)
	{
		if (!multiview)
		{
			graphicswidget_y->clear_();
			graphicswidget_x->clear_();
		}
		histogramview->update__(v);
	}
	else
	{
		histogramview->clear__();
	}
	update_toolbox(v);
	imagesbox->set_html(v);
	set_contourstable(v);
}

void Aliza::update_selection_common2(QListWidgetItem * s)
{
	if (!s)
	{
		clear_views();
		return;
	}
	const bool ok3d = check_3d();
	QList<ImageVariant*> spect_images;
	QList<const ImageVariant*> tmp_images;
	QList<double> deltas;
	ListWidgetItem2 * k = static_cast<ListWidgetItem2*>(s);
	ImageVariant * v = k->get_image_from_item();
	if (v)
	{
		selected_images.push_back(v);
		if (v->image_type == 300) spect_images.push_back(v);
		else tmp_images.push_back(k->get_image_from_item_const());
		if (ok3d) deltas.push_back(CommonUtils::calculate_max_delta(v));
	}
	for (int x = 0; x < imagesbox->listWidget->count(); ++x)
	{
		QListWidgetItem * j = imagesbox->listWidget->item(x);
		if (!j) continue;
		if (Qt::Checked == j->checkState() && j != s)
		{
			ListWidgetItem2 * k1 = static_cast<ListWidgetItem2*>(j);
			if (k1)
			{
				ImageVariant * v1 = k1->get_image_from_item();
				if (v1)
				{
					selected_images.push_back(v1);
					if (v1->image_type == 300) spect_images.push_back(v1);
					else tmp_images.push_back(k1->get_image_from_item_const());
					if (ok3d) deltas.push_back(CommonUtils::calculate_max_delta(v1));
				}
			}
		}
	}
	for (int x = 0; x < spect_images.size(); ++x)
	{
		bool ref_ok{};
		for (int k1 = 0; k1 < tmp_images.size(); ++k1)
		{
			if (tmp_images.at(k1)->frame_of_ref_uid == spect_images.at(x)->frame_of_ref_uid)
			{
				ref_ok = true;
			}
			else
			{
				ref_ok = false;
				break;
			}
		}
		if (ref_ok) spect_images[x]->di->spectroscopy_ref = 1;
		else spect_images[x]->di->spectroscopy_ref = 0;
	}
	spect_images.clear();
	tmp_images.clear();
	if (selected_images.empty())
	{
		clear_views();
	}
	else
	{
		check_slice_collisions(const_cast<const ImageVariant *>(v), graphicswidget_m);
		if (ok3d)
		{
			double max_delta{};
			for (int x = 0; x < deltas.size(); ++x)
			{
				if (deltas.at(x) > max_delta) max_delta = deltas.at(x);
			}
			disconnect(toolbox->far_doubleSpinBox, SIGNAL(valueChanged(double)), glwidget, SLOT(set_far(double)));
			const double far_plane = 7 * max_delta;
			glwidget->update_far_plane(static_cast<float>(far_plane));
			toolbox->far_doubleSpinBox->setValue(far_plane);
			connect(toolbox->far_doubleSpinBox, SIGNAL(valueChanged(double)), glwidget, SLOT(set_far(double)));
		}
		calculate_bb();
	}
}

void Aliza::clear_views()
{
	update_toolbox(nullptr);
	imagesbox->set_html(nullptr);
	clear_contourstable();
	graphicswidget_m->clear_();
	graphicswidget_y->clear_();
	graphicswidget_x->clear_();
	if (
#if 0
		!run__ &&
#endif
		check_3d() && check_3d_visible())
	{
		glwidget->updateGL();
	}
	histogramview->clear__();
}

void Aliza::update_visible_rois(QTableWidgetItem * i)
{
	if (lock0) return;
	lock0 = true;
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	qApp->processEvents();
	ImageVariant * v = get_selected_image();
	if (v)
	{
		const bool ok3d = check_3d();
		if (ok3d) glwidget->set_skip_draw(true);
		TableWidgetItem2 * item = static_cast<TableWidgetItem2*>(i);
		if (item)
		{
			const int id = item->get_id();
			const bool show = (Qt::Checked == item->checkState()) ? true : false;
			for (int x = 0; x < v->di->rois.size(); ++x)
			{
				if (v->di->rois.at(x).id == id)
				{
					v->di->rois[x].show = show;
					break;
				}
			}
		}
		graphicswidget_m->update_image(0, true);
		if (ok3d) glwidget->set_skip_draw(false);
	}
	QApplication::restoreOverrideCursor();
	lock0 = false;
}

void Aliza::reset_rect2()
{
	const bool ok3d = check_3d();
	if (ok3d) glwidget->set_skip_draw(true);
	ImageVariant * v = get_selected_image();
	if (v)
	{
		v->di->from_slice = 0;
		v->di->to_slice = v->di->idimz - 1;
		v->di->irect_index[0] = v->di->irect_index[1] = 0;
		v->di->irect_size[0] = v->di->idimx;
		v->di->irect_size[1] = v->di->idimy;
		update_selection2();
	}
	if (ok3d) glwidget->set_skip_draw(false);
}

void Aliza::reset_level()
{
	const ImageVariant * v = get_selected_image_const();
	if (!v) return;
	toolbox2D->blockSignals(true);
	toolbox2D->set_center(v->di->default_us_window_center);
	toolbox2D->set_width(v->di->default_us_window_width);
	toolbox2D->blockSignals(false);
	toolbox2D->set_lut_function(v->di->default_lut_function);
}

void Aliza::start_anim()
{
	if (lock0 || lock2) return;
	lock2 = true;
	lock0 = true;
	graphicswidget_m->run__ = true;
	slider_m->slices_slider->setEnabled(false);
	toolbox2D->resetlevel_pushButton->setEnabled(false);
	disconnect(
		slider_m->slices_slider, SIGNAL(valueChanged(int)),
		this, SLOT(set_selected_slice2D_m(int)));
	toolbox2D->width_horizontalSlider->setEnabled(false);
	toolbox2D->width_doubleSpinBox->setEnabled(false);
	toolbox2D->width_label->setEnabled(false);
	toolbox2D->center_horizontalSlider->setEnabled(false);
	toolbox2D->center_doubleSpinBox->setEnabled(false);
	toolbox2D->center_label->setEnabled(false);
	toolbox2D->comboBox->setEnabled(false);
	toolbox2D->maxwin_pushButton->hide();
	saved_mouse_modus = graphicswidget_m->get_mouse_modus();
	saved_show_cursor = graphicswidget_m->get_show_cursor();
	frames2DAct->setEnabled(false);
	cursorAct->setEnabled(false);
	collisionAct->setEnabled(false);
	distanceAct->setEnabled(false);
	graphicswidget_m->set_mouse_modus(0, false);
	graphicswidget_y->set_mouse_modus(0, false);
	graphicswidget_x->set_mouse_modus(0, false);
	graphicswidget_m->update_pixel_value(-1, -1);
	graphicswidget_y->update_pixel_value(-1, -1);
	graphicswidget_x->update_pixel_value(-1, -1);
	graphicswidget_m->set_show_cursor(false);
	graphicswidget_y->set_show_cursor(false);
	graphicswidget_x->set_show_cursor(false);
	rectAct->setIcon(cut_icon);
	graphicsAct_Z->setEnabled(false);
	histogramAct->setEnabled(false);
	graphicswidget_m->set_frame_time_unit(settingswidget->get_time_unit());
	graphicswidget_m->start_animation();
}

void Aliza::stop_anim()
{
	if (!graphicswidget_m->run__)
	{
		graphicswidget_m->stop_animation();
		return;
	}
	graphicswidget_m->stop_animation();
	graphicsAct_Z->setEnabled(true);
	histogramAct->setEnabled(true);
	graphicswidget_m->set_mouse_modus(saved_mouse_modus, true);
	graphicswidget_y->set_mouse_modus(saved_mouse_modus, false);
	graphicswidget_x->set_mouse_modus(saved_mouse_modus, false);
	graphicswidget_m->update_pixel_value(-1, -1);
	graphicswidget_y->update_pixel_value(-1, -1);
	graphicswidget_x->update_pixel_value(-1, -1);
	graphicswidget_m->set_show_cursor(saved_show_cursor);
	graphicswidget_y->set_show_cursor(saved_show_cursor);
	graphicswidget_x->set_show_cursor(saved_show_cursor);
	if (saved_mouse_modus == 1)
	{
		graphicswidget_m->update_frames();
		graphicswidget_y->update_frames();
		graphicswidget_x->update_frames();
	}
	if (saved_mouse_modus == 1 || saved_mouse_modus == 2)
	{
		rectAct->setIcon(nocut_icon);
	}
	frames2DAct->setEnabled(true);
	cursorAct->setEnabled(true);
	collisionAct->setEnabled(true);
	distanceAct->setEnabled(true);
	toolbox2D->resetlevel_pushButton->setEnabled(true);
	slider_m->slices_slider->setEnabled(true);
	toolbox2D->maxwin_pushButton->show();
	toolbox2D->comboBox->setEnabled(true);
	toolbox2D->center_horizontalSlider->setEnabled(true);
	toolbox2D->center_doubleSpinBox->setEnabled(true);
	toolbox2D->center_label->setEnabled(true);
	toolbox2D->width_horizontalSlider->setEnabled(true);
	toolbox2D->width_doubleSpinBox->setEnabled(true);
	toolbox2D->width_label->setEnabled(true);
	if (check_3d() && check_3d_visible()) glwidget->updateGL();
	connect(
		slider_m->slices_slider, SIGNAL(valueChanged(int)),
		this, SLOT(set_selected_slice2D_m(int)));
	lock2 = false;
	lock0 = false;
}

void Aliza::zoom_plus_3d()
{
	if (check_3d()) glwidget->zoom_in(true);
}

void Aliza::zoom_minus_3d()
{
	if (check_3d()) glwidget->zoom_out(true);
}

void Aliza::update_slice_from_animation(const ImageVariant * v)
{
	const int a = graphicswidget_m->get_axis();
	switch (a)
	{
	case 0:
		slider_m->set_slice(v->di->selected_x_slice);
		break;
	case 1:
		slider_m->set_slice(v->di->selected_y_slice);
		break;
	case 2:
		slider_m->set_slice(v->di->selected_z_slice);
		break;
	}
	if (a == 2)
	{
		check_slice_collisions(v, graphicswidget_m);
		if (v->di->lock_2Dview)
		{
			zrangewidget->spanslider->blockSignals(true);
			zrangewidget->set_span(v->di->from_slice, v->di->to_slice);
			zrangewidget->spanslider->blockSignals(false);
			if (check_3d() && check_3d_visible()) glwidget->updateGL();
			if (multiview)
			{
				graphicswidget_y->update_selection_item();
				graphicswidget_x->update_selection_item();
			}
		}
	}
}

void Aliza::set_hide_zoom(bool t)
{
	hide_zoom = t;
}

void Aliza::set_histogram_mode(bool t)
{
	histogram_mode = t;
}

void Aliza::trigger_reload_histogram()
{
	if (lock0) return;
	lock0 = true;
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	qApp->processEvents();
	ImageVariant * v = get_selected_image();
	if (v)
	{
		add_histogram(v, nullptr, false);
		histogramview->update__(v);
	}
	QApplication::restoreOverrideCursor();
	lock0 = false;
}

void Aliza::width_from_histogram_min(double x)
{
	ImageVariant * v = get_selected_image();
	if (!v) return;
	toolbox2D->disconnect_sliders();
	disconnect(toolbox2D->width_doubleSpinBox, SIGNAL(valueChanged(double)),this,SLOT(width_from_spinbox(double)));
	disconnect(toolbox2D->center_doubleSpinBox,SIGNAL(valueChanged(double)),this,SLOT(center_from_spinbox(double)));
	const double div_ = v->di->rmax - v->di->rmin;
	double new_width = x;
	if (new_width <= 0) new_width = 1e-9;
	else if (new_width > 1.0) new_width = 1.0;
	const double tmp1 = new_width * div_;
	double old_min = v->di->window_center - v->di->window_width*0.5;
	double new_min = old_min - (new_width - v->di->window_width);
	double new_center = new_min + new_width * 0.5;
	if (new_center < 0.0) new_center = 0.0;
	else if (new_center > 1.0) new_center = 1.0;
	double tmp2 = new_center * div_ + v->di->rmin;
	if (tmp2 > v->di->rmax) tmp2 = v->di->rmax;
	else if (tmp2 < v->di->rmin) tmp2 = v->di->rmin;
	v->di->window_center = new_center;
	v->di->window_width  = new_width;
	v->di->us_window_center = tmp2;
	v->di->us_window_width  = tmp1;
	if (v->group_id >= 0) update_group_width(v);
	toolbox2D->width_doubleSpinBox->setValue(v->di->us_window_width);
	toolbox2D->center_doubleSpinBox->setValue(v->di->us_window_center);
	toolbox2D->width_horizontalSlider->setValue(static_cast<int>(v->di->us_window_width));
	toolbox2D->center_horizontalSlider->setValue(static_cast<int>(v->di->us_window_center));
#if 0
	if (!run__)
#endif
	{
		if (check_3d() && check_3d_visible()) glwidget->updateGL();
		if (!graphicswidget_m->run__) graphicswidget_m->update_image(0, false);
		if (multiview)
		{
			graphicswidget_y->update_image(0, false);
			graphicswidget_x->update_image(0, false);
		}
	}
	connect(toolbox2D->center_doubleSpinBox,SIGNAL(valueChanged(double)),this,SLOT(center_from_spinbox(double)));
	connect(toolbox2D->width_doubleSpinBox, SIGNAL(valueChanged(double)),this,SLOT(width_from_spinbox(double)));
	toolbox2D->connect_sliders();
}

void Aliza::width_from_histogram_max(double x)
{
	ImageVariant * v = get_selected_image();
	if (!v) return;
	if (!toolbox2D) return;
	toolbox2D->disconnect_sliders();
	disconnect(toolbox2D->width_doubleSpinBox, SIGNAL(valueChanged(double)),this,SLOT(width_from_spinbox(double)));
	disconnect(toolbox2D->center_doubleSpinBox,SIGNAL(valueChanged(double)),this,SLOT(center_from_spinbox(double)));
	const double div_ = v->di->rmax - v->di->rmin;
	double new_width = x;
	if (new_width <= 0) new_width = 1e-9;
	else if (new_width > 1.0) new_width = 1.0;
	const double tmp1 = new_width * div_;
	double old_max = v->di->window_center + v->di->window_width * 0.5;
	double new_max = old_max + (new_width - v->di->window_width);
	double new_center = new_max-new_width * 0.5;
	if (new_center < 0.0) new_center = 0.0;
	else if (new_center > 1.0) new_center = 1.0;
	double tmp2 = new_center * div_ + v->di->rmin;
	if (tmp2 > v->di->rmax) tmp2 = v->di->rmax;
	else if (tmp2 < v->di->rmin) tmp2 = v->di->rmin;
	v->di->window_center = new_center;
	v->di->window_width = new_width;
	v->di->us_window_center = tmp2;
	v->di->us_window_width  = tmp1;
	if (v->group_id >= 0) update_group_width(v);
	toolbox2D->width_doubleSpinBox->setValue(v->di->us_window_width);
	toolbox2D->center_doubleSpinBox->setValue(v->di->us_window_center);
	toolbox2D->width_horizontalSlider->setValue(static_cast<int>(v->di->us_window_width));
	toolbox2D->center_horizontalSlider->setValue(static_cast<int>(v->di->us_window_center));
#if 0
	if (!run__)
#endif
	{
		if (check_3d() && check_3d_visible()) glwidget->updateGL();
		if (!graphicswidget_m->run__) graphicswidget_m->update_image(0, false);
		if (multiview)
		{
			graphicswidget_y->update_image(0, false);
			graphicswidget_x->update_image(0, false);
		}
	}
	connect(toolbox2D->center_doubleSpinBox,SIGNAL(valueChanged(double)),this,SLOT(center_from_spinbox(double)));
	connect(toolbox2D->width_doubleSpinBox, SIGNAL(valueChanged(double)),this,SLOT(width_from_spinbox(double)));
	toolbox2D->connect_sliders();
}

void Aliza::center_from_histogram(double x)
{
	ImageVariant * v = get_selected_image();
	if (!v) return;
	toolbox2D->disconnect_sliders();
	disconnect(toolbox2D->center_doubleSpinBox,SIGNAL(valueChanged(double)),this,SLOT(center_from_spinbox(double)));
	const double div_ = v->di->rmax - v->di->rmin;
	double new_center = x;
	if (new_center < 0.0) new_center = 0.0;
	if (new_center > 1.0) new_center = 1.0;
	v->di->us_window_center = new_center*div_+v->di->rmin;
	v->di->window_center = new_center;
	v->di->window_width = div_ > 0 ? v->di->us_window_width/div_ : 1e-9;
	if (v->group_id >= 0) update_group_center(v);
	toolbox2D->center_doubleSpinBox->setValue(v->di->us_window_center);
	toolbox2D->center_horizontalSlider->setValue(static_cast<int>(v->di->us_window_center));
#if 0
	if (!run__)
#endif
	{
		if (check_3d() && check_3d_visible()) glwidget->updateGL();
		if (!graphicswidget_m->run__) graphicswidget_m->update_image(0, false);
		if (multiview)
		{
			graphicswidget_y->update_image(0, false);
			graphicswidget_x->update_image(0, false);
		}
	}
	connect(toolbox2D->center_doubleSpinBox,SIGNAL(valueChanged(double)),this,SLOT(center_from_spinbox(double)));
	toolbox2D->connect_sliders();
}

QString Aliza::create_group_(bool * ok, bool lock_)
{
	if (lock_)
	{
		if (lock0)
		{
			*ok = false;
			return QString("");
		}
		lock0 = true;
	}
	QString message_;
	bool ok_{};
	int group_id{-1};
	int dimx{};
	int dimy{};
	int dimz{};
	short image_type{-1};
	QList<ImageVariant*> tmp_images;
	QList<ImageVariant*> group_images;
	ImageVariant * v;
	QMap<int, ImageVariant*> map;
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	QMap<int, ImageVariant*>::const_iterator iv = scene3dimages.cbegin();
#else
	QMap<int, ImageVariant*>::const_iterator iv = scene3dimages.constBegin();
#endif
	if (scene3dimages.size() < 2)
	{
		message_ = QString(
			"At least 2 3D images are required to create 4D group");
		goto quit__;
	}
	v = get_selected_image();
	if (!v)
	{
		message_ = QString("No image is selected");
		goto quit__;
	}
	if (v->group_id >= 0)
	{
		message_ =
			QString("Image has the group ID ") +
			QVariant(v->group_id).toString() +
			QString(",\n remove group IDs first.");
		goto quit__;
	}
	dimx = v->di->idimx;
	dimy = v->di->idimy;
	dimz = v->di->idimz;
	image_type = v->image_type;
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	while (iv != scene3dimages.cend())
#else
	while (iv != scene3dimages.constEnd())
#endif
	{
		ImageVariant * v2 = iv.value();
		if (v2 &&
			v2->group_id == -1 &&
			v2->di->idimx == dimx &&
			v2->di->idimy == dimy &&
			v2->di->idimz == dimz &&
			v2->image_type == image_type)
		{
			tmp_images.push_back(v2);
		}
		++iv;
	}
	if (tmp_images.size() < 2)
	{
		message_ = QString("Failed: < 2 images can be assigned");
		goto quit__;
	}
	{
		for (int x = 0; x < tmp_images.size(); ++x)
		{
			map[tmp_images.at(x)->id] = tmp_images.at(x);
		}
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
		QMap<int, ImageVariant*>::const_iterator it = map.cbegin();
		while (it != map.cend())
#else
		QMap<int, ImageVariant*>::const_iterator it = map.constBegin();
		while (it != map.constEnd())
#endif
		{
			group_images.push_back(it.value());
			++it;
		}
	}
	group_id = CommonUtils::get_next_group_id();
	for (int z = 0; z < group_images.size(); ++z)
	{
		if (group_images.at(z))
		{
			group_images[z]->group_id = group_id;
			group_images[z]->di->selected_lut  = v->di->selected_lut;
			group_images[z]->di->transparency  = v->di->transparency;
			int tmp0 = v->di->us_window_center;
			if (tmp0 > group_images.at(z)->di->rmax)
				tmp0 = tmp0>group_images.at(z)->di->rmax;
			if (tmp0 < group_images.at(z)->di->rmin)
				tmp0 = tmp0>group_images.at(z)->di->rmin;
			double tmp1 =
				(static_cast<double>(tmp0) +
					(-group_images.at(z)->di->rmin)) /
					(group_images.at(z)->di->rmax-group_images.at(z)->di->rmin);
			if (tmp1 < 0.0) tmp1 = 0.0;
			if (tmp1 > 1.0) tmp1 = 1.0;
			int tmp2 = v->di->us_window_width;
			if (tmp2 > (group_images.at(z)->di->rmax-group_images.at(z)->di->rmin))
				tmp2 = group_images.at(z)->di->rmax-group_images.at(z)->di->rmin;
			double tmp3 =
				static_cast<double>(tmp2) /
				(group_images.at(z)->di->rmax - group_images.at(z)->di->rmin);
			if (tmp3 <= 0.0) tmp3 = 1e-9;
			if (tmp3 >  1.0) tmp3 = 1.0;
			group_images[z]->di->us_window_center = tmp0;
			group_images[z]->di->window_center    = tmp1;
			group_images[z]->di->us_window_width  = tmp2;
			group_images[z]->di->window_width     = tmp3;
			group_images[z]->di->selected_z_slice = v->di->selected_z_slice;
			group_images[z]->di->selected_y_slice = v->di->selected_y_slice;
			group_images[z]->di->selected_x_slice = v->di->selected_x_slice;
		}
	}
	ok_ = true;
	calculate_bb();
	message_=
		QString("Assigned ") +
		QVariant(group_images.size()).toString() +
		QString(" images to 4D group (ID ") +
		QVariant(group_id).toString() +
		QString(")");
quit__:
	tmp_images.clear();
	group_images.clear();
	map.clear();
	*ok = ok_;
	if (lock_) lock0 = false;
	return message_;
}

void Aliza::create_group()
{
	bool ok{};
	QString message_ = create_group_(&ok, true);
	if (!message_.isEmpty())
	{
		QMessageBox mbox;
		mbox.addButton(QMessageBox::Close);
		mbox.setIcon(QMessageBox::Information);
		mbox.setText(message_);
		mbox.exec();
	}
}

void Aliza::remove_group()
{
	if (lock0) return;
	lock0 = true;
	const ImageVariant * v = get_selected_image();
	if (!v)
	{
		lock0 = false;;
		return;
	}
	if (v->group_id < 0)
	{
		QMessageBox mbox;
		mbox.addButton(QMessageBox::Close);
		mbox.setIcon(QMessageBox::Information);
		mbox.setText(QString("Group ID is not set"));
		mbox.exec();
	}
	else
	{
		QMessageBox mbox;
		mbox.addButton(QMessageBox::YesToAll);
		mbox.addButton(QMessageBox::Yes);
		mbox.addButton(QMessageBox::Cancel);
		mbox.setDefaultButton(QMessageBox::Yes);
		mbox.setIcon(QMessageBox::Question);
		mbox.setText(
			QString("Remove 4D group ID ") +
			QVariant(v->group_id).toString() +
			QString(
				"?\nYes to all will remove all group IDs\n"
				"from all images."));
		const int q = mbox.exec();
		if (q == QMessageBox::YesToAll)
		{
			QMap<int, ImageVariant*>::iterator iv = scene3dimages.begin();
			while (iv != scene3dimages.end())
			{
				ImageVariant * v2 = iv.value();
				if (v2 && v2->group_id >= 0)
				{
					v2->group_id = -1;
					set_us_center(v2, v2->di->default_us_window_center);
					set_us_width( v2, v2->di->default_us_window_width);
				}
				++iv;
			}
		}
		else if (q == QMessageBox::Yes)
		{
			const int group_id = v->group_id;
			QMap<int, ImageVariant*>::iterator iv = scene3dimages.begin();
			while (iv != scene3dimages.end())
			{
				ImageVariant * v2 = iv.value();
				if (v2 && v2->group_id == group_id)
				{
					v2->group_id = -1;
					set_us_center(v2, v2->di->default_us_window_center);
					set_us_width( v2, v2->di->default_us_window_width);
				}
				++iv;
			}
		}
	}
	update_selection();
	lock0 = false;
}

void Aliza::start_3D_anim()
{
	if (lock0 || lock3) return;
	lock3 = true;
	lock0 = true;
	run__ = true;
	animation_images.clear();
	anim3d_times.clear();
	anim_idx = 0;
	const ImageVariant * v = get_selected_image_const();
	if (v)
	{
		QMap<int, ImageVariant*> map;
		QMap<int, ImageVariant*> instance_numbers_tmp;
		QMap<QString, ImageVariant*> acq_times_tmp;
		sort_4d(
			animation_images,
			anim3d_times,
			map,
			instance_numbers_tmp,
			acq_times_tmp,
			v->group_id,
			true,
			v->di->selected_x_slice,
			v->di->selected_y_slice,
			v->di->selected_z_slice);
		for (int x = 0; x < animation_images.size(); ++x)
		{
			if (animation_images.at(x) && (v->id == animation_images.at(x)->id))
			{
				anim_idx = x;
				break;
			}
		}
		frames2DAct->setEnabled(false);
		cursorAct->setEnabled(false);
		collisionAct->setEnabled(false);
		distanceAct->setEnabled(false);
		zlockAct->setEnabled(false);
		oneAct->setEnabled(false);
		toolbox2D->maxwin_pushButton->hide();
		saved_mouse_modus = graphicswidget_m->get_mouse_modus();
		saved_show_cursor = graphicswidget_m->get_show_cursor();
		const short mm = (saved_mouse_modus == 5) ? 5 : 0;
		if (mm == 0) rectAct->setIcon(cut_icon);
		else rectAct->setIcon(nocut_icon);
		graphicswidget_m->set_mouse_modus(mm, false);
		graphicswidget_y->set_mouse_modus(mm, false);
		graphicswidget_x->set_mouse_modus(mm, false);
		graphicswidget_m->set_show_cursor(false);
		graphicswidget_y->set_show_cursor(false);
		graphicswidget_x->set_show_cursor(false);
		graphicswidget_m->update_pixel_value(-1, -1);
		graphicswidget_y->update_pixel_value(-1, -1);
		graphicswidget_x->update_pixel_value(-1, -1);
		animate_();
	}
}

void Aliza::stop_3D_anim()
{
	anim3D_timer->stop();
	if (!run__) return;
	const bool ok3d = check_3d();
	if (ok3d) glwidget->set_skip_draw(true);
	run__ = false;
	anim_idx = 0;
	animation_images.clear();
	anim3d_times.clear();
	const short mm = saved_mouse_modus;
	graphicswidget_m->set_mouse_modus(mm, true);
	graphicswidget_y->set_mouse_modus(saved_mouse_modus, false);
	graphicswidget_x->set_mouse_modus(saved_mouse_modus, false);
	graphicswidget_m->update_pixel_value(-1, -1);
	graphicswidget_y->update_pixel_value(-1, -1);
	graphicswidget_x->update_pixel_value(-1, -1);
	graphicswidget_m->set_show_cursor(saved_show_cursor);
	graphicswidget_y->set_show_cursor(saved_show_cursor);
	graphicswidget_x->set_show_cursor(saved_show_cursor);
	if (mm == 1 || mm == 2 || mm == 4 || mm == 5)
	{
		rectAct->setIcon(nocut_icon);
	}
	else
	{
		rectAct->setIcon(cut_icon);
	}
	anim3Dwidget->acq_spinBox->clear();
	frames2DAct->setEnabled(true);
	cursorAct->setEnabled(true);
	collisionAct->setEnabled(true);
	distanceAct->setEnabled(true);
	zlockAct->setEnabled(true);
	if (zlockAct->isChecked()) oneAct->setEnabled(true);
	toolbox2D->maxwin_pushButton->show();
	update_selection();
	lock3 = false;
	lock0 = false;
	if (ok3d) glwidget->set_skip_draw(false);
}

void Aliza::animate_()
{
	if (!run__) return;
	if (animation_images.size() < 2) return;
	const qint64 t0 = QDateTime::currentMSecsSinceEpoch();
	const int tmp0 = anim_idx + 1;
	anim_idx = (tmp0 >= animation_images.size() || tmp0 < 0) ? 0 : tmp0;
	selected_images.clear();
	selected_images.push_back(animation_images.at(anim_idx));
	if (check_3d() && check_3d_visible()) glwidget->updateGL();
	if (check_2d_visible())
	{
		graphicswidget_m->set_slice_2D(animation_images[anim_idx], 0, false);
		if (multiview)
		{
			graphicswidget_y->set_slice_2D(animation_images[anim_idx], 0, false);
			graphicswidget_x->set_slice_2D(animation_images[anim_idx], 0, false);
		}
	}
	const qint64 t1 = QDateTime::currentMSecsSinceEpoch();
	const int dt = static_cast<int>(t1 - t0);
	const bool acq_time =
		anim3Dwidget->t_checkBox->isChecked() && (anim3d_times.size() == animation_images.size());
	const int t = (acq_time)
		? static_cast<int>(round(anim3d_times.at(anim_idx))) - dt
		: frametime_3D - dt;
	if (t <= 0)
	{
		// can not run at required speed
		anim3D_timer->start(2);
		if (!toolbox2D->is_red()) toolbox2D->set_indicator_red();
	}
	else
	{
		anim3D_timer->start(t);
		if (acq_time)
		{
			if (!toolbox2D->is_green()) toolbox2D->set_indicator_green();
		}
		else
		{
			if (!toolbox2D->is_blue()) toolbox2D->set_indicator_blue();
		}
	}
	if (acq_time)
	{
		const int i = static_cast<int>(round(anim3d_times.at(anim_idx)));
		anim3Dwidget->acq_spinBox->setValue(i);
	}
}

bool Aliza::is_animation_running() const
{
	return run__;
}

void Aliza::set_frametime_3D(int x)
{
	frametime_3D = x;
}

int Aliza::get_num_images() const
{
	return scene3dimages.size();
}

void Aliza::update_group_width(const ImageVariant * v)
{
	if (!v) return;
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	QMap<int, ImageVariant*>::const_iterator iv = scene3dimages.cbegin();
	while (iv != scene3dimages.cend())
#else
	QMap<int, ImageVariant*>::const_iterator iv = scene3dimages.constBegin();
	while (iv != scene3dimages.constEnd())
#endif
	{
		ImageVariant * v2 = iv.value();
		if (v2 && (v->group_id == v2->group_id))
		{
			double tmp3 = v->di->us_window_center;
			if (tmp3 > v2->di->rmax) tmp3 = tmp3>v2->di->rmax;
			if (tmp3 < v2->di->rmin) tmp3 = tmp3>v2->di->rmin;
			double tmp4 = (tmp3 + (-v2->di->rmin)) / (v2->di->rmax - v2->di->rmin);
			if (tmp4 < 0.0) tmp4 = 0.0;
			if (tmp4 > 1.0) tmp4 = 1.0;
			v2->di->us_window_center = tmp3;
			v2->di->window_center = tmp4;
			double tmp5 = v->di->us_window_width;
			double div2_ = v2->di->rmax - v2->di->rmin;
			if (div2_ <= 0) div2_ = 1e-9;
			double tmp6 = tmp5/div2_;
			if (tmp6 <= 0) tmp6 = 1e-9;
			else if (tmp6 > 1.0) tmp6 = 1.0;
			v2->di->us_window_width = tmp5;
			v2->di->window_width = tmp6;
		}
		++iv;
	}
}

void Aliza::update_group_center(const ImageVariant * v)
{
	if (!v) return;
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	QMap<int, ImageVariant*>::const_iterator iv = scene3dimages.cbegin();
	while (iv != scene3dimages.cend())
#else
	QMap<int, ImageVariant*>::const_iterator iv = scene3dimages.constBegin();
	while (iv != scene3dimages.constEnd())
#endif
	{
		ImageVariant * v2 = iv.value();
		if (v2 && (v->group_id == v2->group_id))
		{
			double tmp0 = v->di->us_window_center;
			if (tmp0 > v2->di->rmax) tmp0 = tmp0>v2->di->rmax;
			if (tmp0 < v2->di->rmin) tmp0 = tmp0>v2->di->rmin;
			double div2_ = v2->di->rmax - v2->di->rmin;
			if (div2_ <= 0) div2_ = 1e-9;
			double tmp1 = (tmp0 + (-v2->di->rmin)) / div2_;
			if (tmp1 < 0.0) tmp1 = 0.0;
			if (tmp1 > 1.0) tmp1 = 1.0;
			v2->di->us_window_center = tmp0;
			v2->di->window_center = tmp1;
		}
		++iv;
	}
}

void Aliza::flipX()
{
	graphicswidget_m->graphicsview->global_flip_x = !graphicswidget_m->graphicsview->global_flip_x;
	if (
#if 0
		!run__ &&
#endif
		!graphicswidget_m->run__)
		graphicswidget_m->update_image(0, false);
}

void Aliza::flipY()
{
	graphicswidget_m->graphicsview->global_flip_y = !graphicswidget_m->graphicsview->global_flip_y;
	if (
#if 0
		!run__ &&
#endif
		!graphicswidget_m->run__)
		graphicswidget_m->update_image(0, false);
}

void Aliza::toggle_maxwindow(bool i)
{
	if (lock0) return;
	lock0 = true;
	ImageVariant * v{};
	const bool ok3d = check_3d();
	if (ok3d) glwidget->set_skip_draw(true);
	v = get_selected_image();
	if (!v) goto quit__;
	if (!(v->image_type == 0 || v->image_type == 1))
	{
		// maxwin_pushButton must be disabled for other types
		goto quit__;
	}
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	qApp->processEvents();
	v->di->maxwindow = i;
	load_3d(v, true, true, false, true);
	if (!v->histogram.isNull()) add_histogram(v, nullptr, false);
	histogramview->update__(v);
	disconnect_tools();
	toolbox2D->disconnect_sliders();
	toolbox2D->set_max_width(v->di->rmax - v->di->rmin + 1.0);
	toolbox2D->set_window_upper(v->di->rmax);
	toolbox2D->set_window_lower(v->di->rmin);
	toolbox2D->set_width(v->di->us_window_width);
	toolbox2D->set_center(v->di->us_window_center);
	toolbox2D->set_maxwindow(v->di->maxwindow);
	toolbox2D->enable_maxwindow(true);
	if (v->di->disable_int_level)
	{
		toolbox2D->center_horizontalSlider->setEnabled(false);
		toolbox2D->width_horizontalSlider->setEnabled(false);
		toolbox2D->center_horizontalSlider->setValue(0);
		toolbox2D->width_horizontalSlider->setValue(0);
	}
	else
	{
		toolbox2D->center_horizontalSlider->setEnabled(true);
		toolbox2D->width_horizontalSlider->setEnabled(true);
		toolbox2D->center_horizontalSlider->setValue(
			static_cast<int>(v->di->us_window_center));
		toolbox2D->width_horizontalSlider->setValue(
			static_cast<int>(v->di->us_window_width));
	}
	toolbox2D->connect_sliders();
	connect_tools();
	graphicswidget_m->set_slice_2D(v, 0, true);
	check_slice_collisions(const_cast<const ImageVariant *>(v), graphicswidget_m);
	if (multiview) graphicswidget_y->set_slice_2D(v, 0, false);
	if (multiview) graphicswidget_x->set_slice_2D(v, 0, false);
	QApplication::restoreOverrideCursor();
quit__:
	if (ok3d) glwidget->set_skip_draw(false);
	lock0 = false;
}

void Aliza::reset_3d()
{
	if (!check_3d()) return;
	glwidget->set_skip_draw(true);
	disconnect(toolbox->fov_doubleSpinBox,   SIGNAL(valueChanged(double)), glwidget, SLOT(set_fov(double)));
	disconnect(toolbox->far_doubleSpinBox,   SIGNAL(valueChanged(double)), glwidget, SLOT(set_far(double)));
	disconnect(toolbox->alpha_doubleSpinBox, SIGNAL(valueChanged(double)), glwidget, SLOT(set_alpha(double)));
	disconnect(toolbox->bright_doubleSpinBox,SIGNAL(valueChanged(double)), glwidget, SLOT(set_brightness(double)));
	disconnect(toolbox->contours_checkBox,   SIGNAL(toggled(bool)),        glwidget, SLOT(set_display_contours(bool)));
	disconnect(toolbox->cube_checkBox,       SIGNAL(toggled(bool)),        glwidget, SLOT(set_cube(bool)));
	glwidget->fov = SCENE_FOV;
	glwidget->alpha = SCENE_ALPHA;
	glwidget->brightness = 1.0f;
	glwidget->pan_x = 0;
	glwidget->pan_y = 0;
	glwidget->camera->reset();
	glwidget->camera->set_position(0.0f, 0.0f, SCENE_POS_Z);
	glwidget->set_cube(true);
	glwidget->set_display_contours(true);
	toolbox->fov_doubleSpinBox->setValue(SCENE_FOV);
	toolbox->alpha_doubleSpinBox->setValue(SCENE_ALPHA);
	toolbox->bright_doubleSpinBox->setValue(1.0);
	toolbox->contours_checkBox->setChecked(true);
	toolbox->cube_checkBox->setChecked(true);
	glwidget->fit_to_screen(get_selected_image_const());
	const double far_plane = 7 * CommonUtils::calculate_max_delta(get_selected_image_const());
	glwidget->update_far_plane(static_cast<float>(far_plane));
	toolbox->far_doubleSpinBox->setValue(far_plane);
	connect(toolbox->fov_doubleSpinBox,   SIGNAL(valueChanged(double)),glwidget,SLOT(set_fov(double)));
	connect(toolbox->far_doubleSpinBox,   SIGNAL(valueChanged(double)),glwidget,SLOT(set_far(double)));
	connect(toolbox->alpha_doubleSpinBox, SIGNAL(valueChanged(double)),glwidget,SLOT(set_alpha(double)));
	connect(toolbox->bright_doubleSpinBox,SIGNAL(valueChanged(double)),glwidget,SLOT(set_brightness(double)));
	connect(toolbox->contours_checkBox,   SIGNAL(toggled(bool)),       glwidget,SLOT(set_display_contours(bool)));
	connect(toolbox->cube_checkBox,       SIGNAL(toggled(bool)),       glwidget,SLOT(set_cube(bool)));
	glwidget->set_skip_draw(false);
}

void Aliza::set_uniq_string(const QString & s)
{
	uniq_string = s;
}

void Aliza::trigger_check_all()
{
	imagesbox->listWidget->blockSignals(true);
	imagesbox->check_all();
	update_selection();
	imagesbox->listWidget->blockSignals(false);
}

void Aliza::trigger_uncheck_all()
{
	imagesbox->listWidget->blockSignals(true);
	imagesbox->uncheck_all();
	update_selection();
	imagesbox->listWidget->blockSignals(false);
}

void Aliza::trigger_tmp()
{
}

void Aliza::sort_4d(
	QList<ImageVariant*> & images,
	QList<double> & times,
	QMap<int, ImageVariant*> &map,
	QMap<int, ImageVariant*>  & instance_numbers_tmp,
	QMap<QString, ImageVariant*> & acq_times_tmp,
	const int group_id,
	const bool animation,
	const int selected_x_slice,
	const int selected_y_slice,
	const int selected_z_slice)
{
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	QMap<int, ImageVariant*>::const_iterator iv = scene3dimages.cbegin();
	while (iv != scene3dimages.cend())
#else
	QMap<int, ImageVariant*>::const_iterator iv = scene3dimages.constBegin();
	while (iv != scene3dimages.constEnd())
#endif
	{
		ImageVariant * v2 = iv.value();
		if (v2 && (group_id == v2->group_id))
		{
			map[v2->id] = v2;
			instance_numbers_tmp[v2->instance_number] = v2;
			if (!v2->acquisition_date.isEmpty() && !v2->acquisition_time.isEmpty())
			{
				const QString dacs = v2->acquisition_date + v2->acquisition_time;
				acq_times_tmp[dacs] = v2;
			}
			if (animation)
			{
				v2->di->selected_x_slice = selected_x_slice;
				v2->di->selected_y_slice = selected_y_slice;
				v2->di->selected_z_slice = selected_z_slice;
			}
		}
		++iv;
	}
	bool acqtimes_valid = (acq_times_tmp.count() == map.count());
	if (acqtimes_valid)
	{
		unsigned int tmp5{};
		long double t0{};
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
		QMap<QString, ImageVariant*>::const_iterator it = acq_times_tmp.cbegin();
		while (it != acq_times_tmp.cend())
#else
		QMap<QString, ImageVariant*>::const_iterator it = acq_times_tmp.constBegin();
		while (it != acq_times_tmp.constEnd())
#endif
		{
			images.push_back(it.value());
			const QString t1s = it.value()->acquisition_date + it.value()->acquisition_time;
#if 0
			std::cout << "t1s = " << t1s.toStdString() << std::endl;
#endif
			long double t1{};
			try
			{
				t1 = std::stold(t1s.toStdString());
			}
			catch (const std::invalid_argument &)
			{
				;
			}
			catch (const std::out_of_range &)
			{
				;
			}
			if (tmp5 > 0 && t1 > 0.0L)
			{
				const long double tdld = (t1 - t0) * 1000.0L;
				const double td = static_cast<double>(tdld);
#if 0
				std::cout << "(t1 - t0) * 1000.0L = " << td << std::endl;
#endif
				times.push_back(td);
			}
			t0 = t1;
			++it;
			++tmp5;
		}
		times.push_back(1000.0);
		if (images.size() == times.size())
		{
#if 0
			std::cout << "Acq. times taken to sort 3D images in 4D image" << std::endl;
#endif
		}
		else
		{
			acqtimes_valid = false;
		}
	}
	if (!acqtimes_valid)
	{
		times.clear();
		images.clear();
		anim3Dwidget->acq_spinBox->clear();
		bool instance_numbers_valid{};
		if (instance_numbers_tmp.count() == map.count())
		{
			instance_numbers_valid = true;
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
			QMap<int, ImageVariant*>::const_iterator it = instance_numbers_tmp.cbegin();
			while (it != instance_numbers_tmp.cend())
#else
			QMap<int, ImageVariant*>::const_iterator it = instance_numbers_tmp.constBegin();
			while (it != instance_numbers_tmp.constEnd())
#endif
			{
				if (it.key() <= 0)
				{
					instance_numbers_valid = false;
					break;
				}
				++it;
			}
		}
		if (instance_numbers_valid)
		{
#if 0
			std::cout
				<< "Instance numbers taken to sort 3D images in 4D image"
				<< std::endl;
#endif
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
			QMap<int, ImageVariant*>::const_iterator it = instance_numbers_tmp.cbegin();
			while (it != instance_numbers_tmp.cend())
#else
			QMap<int, ImageVariant*>::const_iterator it = instance_numbers_tmp.constBegin();
			while (it != instance_numbers_tmp.constEnd())
#endif
			{
				images.push_back(it.value());
				++it;
			}
		}
		else
		{
#if 0
			std::cout
				<< "IDs taken to sort 3D images in 4D image"
				<< std::endl;
#endif
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
			QMap<int, ImageVariant*>::const_iterator it = map.cbegin();
			while (it != map.cend())
#else
			QMap<int, ImageVariant*>::const_iterator it = map.constBegin();
			while (it != map.constEnd())
#endif
			{
				images.push_back(it.value());
				++it;
			}
		}
	}
#if 0
	for (int x = 0; x < images.size(); ++x)
	{
		std::cout << "id=" << images.at(x)->id
			<< " acq. date: " << images.at(x)->acquisition_date.toStdString()
			<< ", acq. time: " << images.at(x)->acquisition_time.toStdString()
			<< ", series date: " << images.at(x)->series_date.toStdString()
			<< ", series time: " << images.at(x)->series_time.toStdString()
			<< std::endl;
	}
#endif
}

void Aliza::toggle_zlock(bool t)
{
	ImageVariant * v = get_selected_image();
	if (!v) return;
	if (v->group_id >= 0)
	{
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
		QMap<int, ImageVariant*>::const_iterator iv = scene3dimages.cbegin();
		while (iv != scene3dimages.cend())
#else
		QMap<int, ImageVariant*>::const_iterator iv = scene3dimages.constBegin();
		while (iv != scene3dimages.constEnd())
#endif
		{
			ImageVariant * v2 = iv.value();
			if (v2 && (v->group_id == v2->group_id))
			{
				v2->di->lock_2Dview = t;
				if (t)
				{
					v2->di->transparency = false;
				}
				else
				{
					v2->di->from_slice = 0;
					v2->di->to_slice = v2->di->idimz > 1 ? v2->di->idimz - 1 : 0;
				}
			}
			++iv;
		}
	}
	else
	{
		v->di->lock_2Dview = t;
		if (t)
		{
			v->di->transparency = false;
		}
		else
		{
			v->di->from_slice = 0;
			v->di->to_slice = v->di->idimz > 1 ? v->di->idimz - 1 : 0;
		}
	}
	update_toolbox(v);
	calculate_bb();
}

void Aliza::toggle_zlock_one(bool t)
{
	ImageVariant * v = get_selected_image();
	if (!v) return;
	if (v->group_id >= 0)
	{
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
		QMap<int, ImageVariant*>::const_iterator iv = scene3dimages.cbegin();
		while (iv != scene3dimages.cend())
#else
		QMap<int, ImageVariant*>::const_iterator iv = scene3dimages.constBegin();
		while (iv != scene3dimages.constEnd())
#endif
		{
			ImageVariant * v2 = iv.value();
			if (v2 && (v->group_id == v2->group_id))
			{
				v2->di->lock_single  = t;
				if (t) v2->di->transparency = false;
			}
			++iv;
		}
	}
	else
	{
		v->di->lock_single = t;
		if (t) v->di->transparency = false;
	}
	update_toolbox(v);
	calculate_bb();
}

void Aliza::toggle_collisions(bool t)
{
	show_all_study_collisions = t;
	check_slice_collisions(get_selected_image_const(), graphicswidget_m);
}

void Aliza::trigger_image_color() 
{
	if (lock0) return;
	lock0 = true;
	QColor old_color;
	QColor new_color;
	QList<QListWidgetItem*> l;
	const bool ok3d = check_3d();
	if (ok3d) glwidget->set_skip_draw(true);
	ImageVariant * v = get_selected_image();
	if (!v) goto quit__;
	old_color = QColor(
		round(v->di->R * 255.0),
		round(v->di->G * 255.0),
		round(v->di->B * 255.0));
	new_color = QColorDialog::getColor(old_color);
	if(new_color.isValid())
	{
		v->di->R = new_color.red()   / 255.0;
		v->di->G = new_color.green() / 255.0;
		v->di->B = new_color.blue()  / 255.0;
		IconUtils::update_icon(v, 96);
		l = imagesbox->listWidget->selectedItems();
		if (!l.empty())
		{
			if (l.at(0)) l[0]->setIcon(v->icon);
		}
		if (studyview)
		{
			const bool scouts = studyview->get_scouts();
			for (int x = 0; x < studyview->widgets.size(); ++x)
			{
				if (studyview->widgets.at(x) && studyview->widgets.at(x)->graphicswidget)
				{
					if (studyview->widgets.at(x)->graphicswidget->image_container.image3D)
					{
						if (studyview->widgets.at(x)->graphicswidget->image_container.image3D->id == v->id)
						{
							studyview->widgets[x]->graphicswidget->update_image_color(
								new_color.red(),
								new_color.green(),
								new_color.blue());
						}
					}
					if (scouts)
					{
						studyview->widgets[x]->graphicswidget->graphicsview->clear_collision_paths();
					}
				}
			}
			if (scouts) check_slice_collisions2(studyview);
		}
	}
quit__:
	if (ok3d) glwidget->set_skip_draw(false);
	lock0 = false;
}

void Aliza::delete_checked_unchecked(bool t)
{
	QList<QListWidgetItem *> items;
	QList<int> image_ids;
	if (lock0) return;
	lock0 = true;
	const bool ok3d = check_3d();
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	qApp->processEvents();
	if (ok3d) glwidget->set_skip_draw(true);
	for (int x = 0; x < imagesbox->listWidget->count(); ++x)
	{
		QListWidgetItem * j = imagesbox->listWidget->item(x);
		if (j)
		{
			if (t && Qt::Checked == j->checkState())
			{
				items.push_back(j);
			}
			else if (!t && Qt::Unchecked == j->checkState())
			{
				items.push_back(j);
			}
		}
	}
	if (items.empty()) goto quit__;
	graphicswidget_m->clear_();
	graphicswidget_y->clear_();
	graphicswidget_x->clear_();
	histogramview->clear__();
	if (studyview) studyview->block_signals(true);
	imagesbox->listWidget->blockSignals(true);
	disconnect(imagesbox->listWidget,SIGNAL(itemSelectionChanged()),this,SLOT(update_selection()));
	disconnect(imagesbox->listWidget,SIGNAL(itemChanged(QListWidgetItem*)),this,SLOT(update_selection()));
	for (int x = 0; x < items.size(); ++x)
	{
		const ListWidgetItem2 * i = static_cast<const ListWidgetItem2*>(items.at(x));
		image_ids.push_back(i->get_id());
	}
	for (int x = 0; x < items.size(); ++x)
	{
		ListWidgetItem2 * i = static_cast<ListWidgetItem2*>(items[x]);
		imagesbox->listWidget->removeItemWidget(items[x]);
		delete i;
	}
	imagesbox->listWidget->reset();
	for (int x = 0; x < image_ids.size(); ++x)
	{
		ImageVariant * ivariant = get_image(image_ids.at(x));
		if (ivariant)
		{
			remove_from_studyview(ivariant->id);
			scene3dimages.remove(ivariant->id);
			delete ivariant;
		}
	}
	imagesbox->listWidget->blockSignals(false);
	if (studyview)
	{
		studyview->block_signals(false);
		if (scene3dimages.empty())
		{
			studyview->clear_();
			if (studyview->get_in_tab()) studyview->calculate_grid(2);
		}
	}
	update_selection();
	connect(imagesbox->listWidget,SIGNAL(itemSelectionChanged()),this,SLOT(update_selection()));
	connect(imagesbox->listWidget,SIGNAL(itemChanged(QListWidgetItem*)),this,SLOT(update_selection()));
	imagesbox->listWidget->blockSignals(false);
quit__:
	if (ok3d) glwidget->set_skip_draw(false);
	QApplication::restoreOverrideCursor();
#ifdef ALIZA_PRINT_COUNT_GL_OBJ
	std::cout << "Num VBOs " << GLWidget::get_count_vbos() << std::endl;
#endif
	lock0 = false;
}

void Aliza::delete_group(const int group_id)
{
	QList<QListWidgetItem *> items;
	QList<int> image_ids;
	for (int x = 0; x < imagesbox->listWidget->count(); ++x)
	{
		QListWidgetItem * i = imagesbox->listWidget->item(x);
		if (i)
		{
			const ListWidgetItem2 * j = static_cast<ListWidgetItem2*>(i);
			const ImageVariant * v = get_image(j->get_id());
			if (v && v->group_id == group_id) items.push_back(i);
		}
	}
	graphicswidget_m->clear_();
	graphicswidget_y->clear_();
	graphicswidget_x->clear_();
	histogramview->clear__();
	if (studyview) studyview->block_signals(true);
	imagesbox->listWidget->blockSignals(true);
	disconnect(imagesbox->listWidget,SIGNAL(itemSelectionChanged()),this,SLOT(update_selection()));
	disconnect(imagesbox->listWidget,SIGNAL(itemChanged(QListWidgetItem*)),this,SLOT(update_selection()));
	for (int x = 0; x < items.size(); ++x)
	{
		const ListWidgetItem2 * i = static_cast<const ListWidgetItem2*>(items.at(x));
		image_ids.push_back(i->get_id());
	}
	for (int x = 0; x < items.size(); ++x)
	{
		ListWidgetItem2 * i = static_cast<ListWidgetItem2*>(items[x]);
		imagesbox->listWidget->removeItemWidget(items[x]);
		delete i;
	}
	imagesbox->listWidget->reset();
	for (int x = 0; x < image_ids.size(); ++x)
	{
		ImageVariant * ivariant = get_image(image_ids.at(x));
		if (ivariant)
		{
			remove_from_studyview(ivariant->id);
			scene3dimages.remove(ivariant->id);
			delete ivariant;
			ivariant = nullptr;
		}
	}
	update_selection();
	connect(imagesbox->listWidget,SIGNAL(itemSelectionChanged()),this,SLOT(update_selection()));
	connect(imagesbox->listWidget,SIGNAL(itemChanged(QListWidgetItem*)),this,SLOT(update_selection()));
	imagesbox->listWidget->blockSignals(false);
	if (studyview)
	{
		studyview->block_signals(false);
		if (scene3dimages.empty())
		{
			studyview->clear_();
			if (studyview->get_in_tab()) studyview->calculate_grid(2);
		}
	}
}

QString Aliza::load_dicom_file(
	const QString & f,
	QProgressDialog * pb)
{
	if (lock0) return QString("");
	lock0 = true;
	QString message;
	std::vector<ImageVariant*> ivariants;
	QStringList pdf_files;
	QStringList stl_files;
	QStringList video_files;
	QStringList spectroscopy_files;
	QStringList sr_files;
	QStringList filenames;
	filenames.push_back(f);
	const bool ok3d = check_3d();
	if (ok3d)
	{
		glwidget->set_skip_draw(true);
	}
	const bool dcm_thread = settingswidget->get_dcm_thread();
	if (dcm_thread)
	{
		LoadDicom_T * lt = new LoadDicom_T(
			QString(""),
			filenames,
			ok3d,
			static_cast<const QWidget * const>(
				const_cast<const SettingsWidget * const>(settingswidget)),
			0,
			settingswidget->get_enh_strategy());
		lt->start();
		while (!lt->isFinished())
		{
			delay1(100);
		}
		const QString message_ = lt->message;
		if (!message_.isEmpty())
		{
			const int filenames_size = filenames.size();
			if (!message.isEmpty()) message.append(QChar('\n'));
			if (filenames_size == 1)
			{
				message.append(filenames.at(0) + QString(":\n    "));
			}
			else if (filenames_size >= 1)
			{
				message.append(filenames.at(0) + QString(" (1st file):\n    "));
			}
			message.append(message_ + QString("\n\n"));
		}
		for (size_t k = 0; k < lt->ivariants.size(); ++k)
		{
			ivariants.push_back(lt->ivariants[k]);
		}
		for (int k = 0; k < lt->pdf_files.size(); ++k)
		{
			pdf_files.push_back(lt->pdf_files.at(k));
		}
		for (int k = 0; k < lt->stl_files.size(); ++k)
		{
			stl_files.push_back(lt->stl_files.at(k));
		}
		for (int k = 0; k < lt->video_files.size(); ++k)
		{
			video_files.push_back(lt->video_files.at(k));
		}
		for (int k = 0; k < lt->spectroscopy_files.size(); ++k)
		{
			spectroscopy_files.push_back(lt->spectroscopy_files.at(k));
		}
		for (int k = 0; k < lt->sr_files.size(); ++k)
		{
			sr_files.push_back(lt->sr_files.at(k));
		}
#if 0
		lt->quit();
		lt->wait();
#endif
		delete lt;
	}
	else
	{
		LoadDicom * lt = new LoadDicom(
			QString(""),
			filenames,
			ok3d,
			static_cast<const QWidget * const>(
				const_cast<const SettingsWidget * const>(settingswidget)),
			0,
			settingswidget->get_enh_strategy());
		lt->run();
		const QString message_ = lt->message;
		if (!message_.isEmpty())
		{
			const int filenames_size = filenames.size();
			if (!message.isEmpty()) message.append(QChar('\n'));
			if (filenames_size == 1)
			{
				message.append(filenames.at(0) + QString(":\n    "));
			}
			else if (filenames_size >= 1)
			{
				message.append(filenames.at(0) + QString(" (1st file):\n    "));
			}
			message.append(message_ + QString("\n\n"));
		}
		for (size_t k = 0; k < lt->ivariants.size(); ++k)
		{
			ivariants.push_back(lt->ivariants[k]);
		}
		for (int k = 0; k < lt->pdf_files.size(); ++k)
		{
			pdf_files.push_back(lt->pdf_files.at(k));
		}
		for (int k = 0; k < lt->stl_files.size(); ++k)
		{
			stl_files.push_back(lt->stl_files.at(k));
		}
		for (int k = 0; k < lt->video_files.size(); ++k)
		{
			video_files.push_back(lt->video_files.at(k));
		}
		for (int k = 0; k < lt->spectroscopy_files.size(); ++k)
		{
			spectroscopy_files.push_back(lt->spectroscopy_files.at(k));
		}
		for (int k = 0; k < lt->sr_files.size(); ++k)
		{
			sr_files.push_back(lt->sr_files.at(k));
		}
		delete lt;
	}
	//
	const QString message_ = process_dicom(
		ivariants,
		pdf_files,
		stl_files,
		video_files,
		spectroscopy_files,
		sr_files,
		pb);
	if (!message_.isEmpty())
	{
		message.append(message_);
	}
	lock0 = false;
	return message;
}

void Aliza::toggle_lock_window(bool t)
{
	toolbox2D->toggle_locked_values(t);
	ImageVariant * v = get_selected_image();
	if (!v) return;
	if (v->group_id >= 0)
	{
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
		QMap<int, ImageVariant*>::const_iterator iv = scene3dimages.cbegin();
		while (iv != scene3dimages.cend())
#else
		QMap<int, ImageVariant*>::const_iterator iv = scene3dimages.constBegin();
		while (iv != scene3dimages.constEnd())
#endif
		{
			ImageVariant * v2 = iv.value();
			if (v2 && (v->group_id == v2->group_id))
			{
				v2->di->lock_level2D = t;
			}
			++iv;
		}
	}
	else
	{
		v->di->lock_level2D = t;
	}
	if (graphicswidget_m->get_axis() == 2)
	{
		set_selected_slice2D_m(v->di->selected_z_slice);
	}
}

void Aliza::clear_contourstable()
{
	imagesbox->contours_tableWidget->blockSignals(true);
	imagesbox->set_contours(nullptr);
	imagesbox->contours_tableWidget->blockSignals(false);
}

void Aliza::set_contourstable(const ImageVariant * v)
{
	imagesbox->contours_tableWidget->blockSignals(true);
	imagesbox->set_contours(v);
	imagesbox->contours_tableWidget->blockSignals(false);
}

void Aliza::trigger_show_roi_info()
{
	if (lock0) return;
	lock0 = true;
	const ImageVariant * v = get_selected_image_const();
	if (!v)
	{
		lock0 = false;
		return;
	}
	int tmp0{-1};
	const int roi_id = imagesbox->get_selected_roi_id();
	if (roi_id < 0)
	{
		lock0 = false;
		return;
	}
	for (int x = 0; x < v->di->rois.size(); ++x)
	{
		if (v->di->rois.at(x).id == roi_id)
		{
			tmp0 = x;
			break;
		}
	}
	if (tmp0 >= 0)
	{
		unsigned int count_contours{};
		bool has_closed_planar{};
		bool has_open_planar{};
		bool has_open_non_planar{};
		bool has_point{};
		bool has_closedplanar_xor{};
		{
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
			QMap<int, Contour*>::const_iterator it = v->di->rois.at(tmp0).contours.cbegin();
			while (it != v->di->rois.at(tmp0).contours.cend())
#else
			QMap<int, Contour*>::const_iterator it = v->di->rois.at(tmp0).contours.constBegin();
			while (it != v->di->rois.at(tmp0).contours.constEnd())
#endif
			{
				const Contour * c = it.value();
				if (c)
				{
					++count_contours;
					const short ct = c->type;
					switch (ct)
					{
					case 1:
						if (!has_closed_planar) has_closed_planar = true;
						break;
					case 2:
						if (!has_open_planar) has_open_planar = true;
						break;
					case 3:
						if (!has_open_non_planar) has_open_non_planar = true;
						break;
					case 4:
						if (!has_point) has_point = true;
						break;
					case 5:
						if (!has_closedplanar_xor) has_closedplanar_xor = true;
						break;
					case 0:
					default:
						break;
					}
				}
				++it;
			}
		}
		{
			QString s0;
			if (has_closed_planar)
			{
				s0.append(QString("CLOSED_PLANAR"));
			}
			if (has_open_planar)
			{
				if (!s0.isEmpty()) s0.append(QString(", "));
				s0.append(QString("OPEN_PLANAR"));
			}
			if (has_open_non_planar)
			{
				if (!s0.isEmpty()) s0.append(QString(", "));
				s0.append(QString("OPEN_NONPLANAR"));
			}
			if (has_point)
			{
				if (!s0.isEmpty()) s0.append(QString(", "));
				s0.append(QString("POINT"));
			}
			if (has_closedplanar_xor)
			{
				if (!s0.isEmpty()) s0.append(QString(", "));
				s0.append(QString("CLOSEDPLANAR_XOR"));
			}
			if (!s0.isEmpty())
			{
				s0.prepend(QString("\nContours type: "));
			}
			const QString s =
				QString("Name: ") +
				v->di->rois.at(tmp0).name +
				QString("\nType: ") +
				v->di->rois.at(tmp0).interpreted_type +
				QString("\nNumber of contours: ") +
				QVariant(count_contours).toString() +
				s0;
			QMessageBox mbox;
			mbox.setWindowTitle("ROI Info");
			mbox.addButton(QMessageBox::Close);
			mbox.setIcon(QMessageBox::Information);
			mbox.setText(s);
			mbox.exec();
		}
	}
	lock0 = false;
}

void Aliza::trigger_studyview()
{
	if (!studyview) return;
	if (lock0) return;
	lock0 = true;
	ImageVariant * v = get_selected_image();
	if (!v)
	{
		QMessageBox::information(nullptr, QString("Information"), QString("Select image"));
		lock0 = false;
		return;
	}
	studyview->clear_();
	if (!studyview->get_in_tab())
	{
		if (studyview->isMinimized())
		{
			studyview->showNormal();
		}
		else
		{
			studyview->show();
		}
#if (defined(__APPLE__) || defined(_WIN32))
		studyview->activateWindow();
#endif
		studyview->raise();
	}
	QList<ImageVariant*> l;
	l.push_back(v);
	QMap<int, ImageVariant*>::iterator it = scene3dimages.begin();
	while (it != scene3dimages.end())
	{
		ImageVariant * v1 = it.value();
		if (v1 && (v1->id != v->id) && (v1->study_uid == v->study_uid))
		{
			l.push_back(v1);
		}
		++it;
	}
	const int n = l.size();
	studyview->calculate_grid(n);
	int x{};
	for (int j = 0; j < n; ++j)
	{
		ImageVariant * v2 = l[j];
		if (v2 && (x < studyview->widgets.size()))
		{
			studyview->widgets[x]->graphicswidget->clear_();
			studyview->widgets[x]->graphicswidget->set_image(v2, 1, true);
		}
		++x;
	}
	check_slice_collisions2(studyview);
	lock0 = false;
}

void Aliza::trigger_studyview_checked()
{
	if (!studyview) return;
	if (lock0) return;
	lock0 = true;
	QList<ImageVariant*> l;
	ImageVariant * v = get_selected_image();
	if (v) l.push_back(v);
	QList<const QListWidgetItem*> items;
	for (int x = 0; x < imagesbox->listWidget->count(); ++x)
	{
		const QListWidgetItem * j = imagesbox->listWidget->item(x);
		if (j)
		{
			if (Qt::Checked == j->checkState()) items.push_back(j);
		}
	}
	for (int x = 0; x < items.size(); ++x)
	{
		const ListWidgetItem2 * i = static_cast<const ListWidgetItem2*>(items.at(x));
		ImageVariant * v1 = get_image(i->get_id());
		if (v1)
		{
			if (v && (v1->id == v->id))
			{
				;
			}
			else
			{
				l.push_back(v1);
			}
		}
	}
	const int n = l.size();
	if (n == 0)
	{
		QMessageBox::information(nullptr, QString("Information"), QString("Nothing selected"));
		lock0 = false;
		return;
	}
	studyview->clear_();
	if (!studyview->get_in_tab())
	{
		if (studyview->isMinimized())
		{
			studyview->showNormal();
		}
		else
		{
			studyview->show();
		}
#if (defined(__APPLE__) || defined(_WIN32))
		studyview->activateWindow();
#endif
		studyview->raise();
	}
	studyview->calculate_grid(n);
	int x{};
	for (int j = 0; j < n; ++j)
	{
		ImageVariant * v1 = l[j];
		if (v1 && (x < studyview->widgets.size()))
		{
			studyview->widgets[x]->graphicswidget->set_image(v1, 1, true);
		}
		++x;
	}
	check_slice_collisions2(studyview);
	lock0 = false;
}

void Aliza::trigger_studyview_empty()
{
	if (!studyview) return;
	if (lock0) return;
	lock0 = true;
	studyview->clear_();
	if (!studyview->get_in_tab())
	{
		if (studyview->isMinimized())
		{
			studyview->showNormal();
		}
		else
		{
			studyview->show();
		}
#if (defined(__APPLE__) || defined(_WIN32))
		studyview->activateWindow();
#endif
		studyview->raise();
	}
	studyview->calculate_grid(2);
	lock0 = false;
}

void Aliza::update_studyview_intersections()
{
	check_slice_collisions2(studyview);
}

void Aliza::remove_from_studyview(int id)
{
	if (!studyview) return;
	for (int x = 0; x < studyview->widgets.size(); ++x)
	{
		if (studyview->widgets.at(x)->graphicswidget->image_container.image3D)
		{
			if (studyview->widgets.at(x)->graphicswidget->image_container.image3D->id == id)
			{
				studyview->widgets[x]->graphicswidget->clear_();
				if (studyview->widgets.at(x)->frame0->frameShape() != QFrame::StyledPanel)
				{
					studyview->widgets[x]->frame0->setFrameShape(QFrame::StyledPanel);
				}
				if (studyview->widgets.at(x)->icon_button->isChecked())
				{
					studyview->widgets[x]->icon_button->blockSignals(true);
					studyview->widgets[x]->icon_button->setChecked(false);
					studyview->widgets[x]->icon_button->blockSignals(false);
				}
			}
		}
	}
	if (studyview->get_active_id() == id)
	{
		studyview->set_active_id(-1);
		studyview->update_null();
	}
	check_slice_collisions2(studyview);
}

QString Aliza::process_dicom(
	std::vector<ImageVariant*> & ivariants,
	const QStringList & pdf_files,
	const QStringList & stl_files,
	const QStringList & video_files,
	const QStringList & spectroscopy_files,
	const QStringList & sr_files,
	QProgressDialog * pb)
{
	QString message;
	int max_3d_tex_size{};
	const bool ok3d = check_3d();
	if (ok3d)
	{
		max_3d_tex_size = glwidget->max_3d_texture_size;
	}
	if (!spectroscopy_files.isEmpty())
	{
		for (int k = 0; k < spectroscopy_files.size(); ++k)
		{
			qApp->processEvents();
			try
			{
				mdcm::Reader reader;
#ifdef _WIN32
#if (defined(_MSC_VER) && defined(MDCM_WIN32_UNC))
				reader.SetFileName(QDir::toNativeSeparators(spectroscopy_files.at(k)).toUtf8().constData());
#else
				reader.SetFileName(QDir::toNativeSeparators(spectroscopy_files.at(k)).toLocal8Bit().constData());
#endif
#else
				reader.SetFileName(spectroscopy_files.at(k).toLocal8Bit().constData());
#endif
				if (!reader.Read()) continue;
				const mdcm::File & file = reader.GetFile();
				const mdcm::DataSet & ds = file.GetDataSet();
				if (ds.IsEmpty()) continue;
				const QString spect_message =
					SpectroscopyUtils::ProcessData(ds, ivariants, max_3d_tex_size, glwidget, ok3d, pb, 0.01f);
				const int filenames_size = spectroscopy_files.size();
				if (!spect_message.isEmpty())
				{
					if (!message.isEmpty()) message.append(QChar('\n'));
					if (filenames_size == 1)
					{
						message.append(spectroscopy_files.at(0) + QString(":\n    "));
					}
					else if (filenames_size >= 1)
					{
						message.append(spectroscopy_files.at(0) + QString(" (1st file):\n    "));
					}
					message.append(spect_message + QString("\n\n"));
				}
			}
			catch (const mdcm::ParseException & pe)
			{
#ifdef ALIZA_VERBOSE
				std::cout << pe.GetLastElement().GetTag() << std::endl;
#else
				(void)pe;
#endif
			}
			catch (const std::exception & ex)
			{
#ifdef ALIZA_VERBOSE
				std::cout << ex.what() << std::endl;
#else
				(void)ex;
#endif
			}
		}
	}
	//
	imagesbox->listWidget->blockSignals(true);
	disconnect(imagesbox->listWidget,SIGNAL(itemSelectionChanged()),this,SLOT(update_selection()));
	disconnect(imagesbox->listWidget,SIGNAL(itemChanged(QListWidgetItem*)),this,SLOT(update_selection()));
	const unsigned int ivariants_size = ivariants.size();
	for (unsigned int x = 0; x < ivariants_size; ++x)
	{
		if (!ivariants.at(x))
		{
#if 0
			std::cout << "ivariants.at(" << x << ") is null" << std::endl;
#endif
			continue;
		}
		if (ivariants.at(x)->di->opengl_ok &&
			!ivariants.at(x)->di->skip_texture && (
				ivariants.at(x)->di->idimz !=
				static_cast<int>(ivariants.at(x)->di->image_slices.size())))
		{
#ifdef ALIZA_VERBOSE
			std::cout
				<< "ivariants.at(" << x
				<< ")->di->idimz != ivariants.at(" << x
				<< ")->di->image_slices.size()" << std::endl;
#endif
			ivariants[x]->di->close();
			ivariants[x]->di->skip_texture = true;
		}
#if 0
		{
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
			QMap<int, QString>::const_iterator it = ivariants.at(x)->image_instance_uids.cbegin();
#else
			QMap<int, QString>::const_iterator it = ivariants.at(x)->image_instance_uids.constBegin();
#endif
			std::cout
				<< "Instance UIDs (dimZ="
				<< ivariants.at(x)->di->idimz <<") :"
				<< std::endl;
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
			while (it != ivariants.at(x)->image_instance_uids.cend())
#else
			while (it != ivariants.at(x)->image_instance_uids.constEnd())
#endif
			{
				std::cout
					<< " " << it.key() << " "
					<< it.value().toStdString()
					<< std::endl;
				++it;
			}
		}
#endif
		add_histogram(ivariants.at(x), pb);
		IconUtils::icon(ivariants[x]);
		//
		if (ivariants.at(x)->di->opengl_ok)
		{
			ivariants[x]->di->set_glwidget(glwidget);
		}
		if (ivariants_size - 1 == x) reload_3d(ivariants[x]);
		else reload_3d(ivariants[x], false);
		//
		{
			ContourUtils::calculate_contours_uv(ivariants[x]);
			ContourUtils::map_contours_all(ivariants[x]);
			ContourUtils::contours_build_path_all(ivariants[x]);
			if (ivariants.at(x)->di->opengl_ok)
			{
				for (int z = 0; z < ivariants.at(x)->di->rois.size(); ++z)
				{
					ContourUtils::generate_roi_vbos(
						glwidget, ivariants[x]->di->rois[z], false);
				}
			}
		}
		//
		scene3dimages[ivariants.at(x)->id] = ivariants[x];
		imagesbox->listWidget->reset();
		imagesbox->add_image(ivariants.at(x)->id, ivariants[x], &ivariants[x]->icon);
		int r{-1};
		for (int j = 0; j < imagesbox->listWidget->count(); ++j)
		{
			int id0{-1};
			ListWidgetItem2 * item = static_cast<ListWidgetItem2*>(imagesbox->listWidget->item(j));
			if (item) id0 = item->get_id();
			if (id0 == ivariants.at(x)->id)
			{
				r = j;
				break;
			}
		}
		if (x == ivariants_size - 1 && r > -1)
		{
			imagesbox->listWidget->setCurrentRow(r);
			update_selection2();
			if (ok3d) glwidget->fit_to_screen(ivariants.at(x));
			emit image_opened();
		}
	}
	connect(imagesbox->listWidget,SIGNAL(itemSelectionChanged()),this,SLOT(update_selection()));
	connect(imagesbox->listWidget,SIGNAL(itemChanged(QListWidgetItem*)),this,SLOT(update_selection()));
	imagesbox->listWidget->blockSignals(false);
	ivariants.clear();
	if (ok3d) glwidget->set_skip_draw(false);
	//
	if (!sr_files.empty())
	{
		for (int k = 0; k < sr_files.size(); ++k)
		{
			qApp->processEvents();
			QFileInfo fi(sr_files.at(k));
			try
			{
				mdcm::Reader reader;
#ifdef _WIN32
#if (defined(_MSC_VER) && defined(MDCM_WIN32_UNC))
				reader.SetFileName(QDir::toNativeSeparators(sr_files.at(k)).toUtf8().constData());
#else
				reader.SetFileName(QDir::toNativeSeparators(sr_files.at(k)).toLocal8Bit().constData());
#endif
#else
				reader.SetFileName(sr_files.at(k).toLocal8Bit().constData());
#endif
				if (!reader.Read()) continue;
				const mdcm::File & file = reader.GetFile();
				const mdcm::DataSet & ds = file.GetDataSet();
				if (ds.IsEmpty()) continue;
				const bool srinfo = settingswidget->get_sr_info();
				QString t00080005;
				const bool t00080005_ok = DicomUtils::get_string_value(
					ds, mdcm::Tag(0x0008,0x0005), t00080005);
				(void)t00080005_ok;
				const QString s0 = SRUtils::read_sr_title1(ds, t00080005);
				SRUtils::set_asked_for_path_once(false);
				SRWidget * sr = new SRWidget(settingswidget->get_scale_icons());
				sr->setAttribute(Qt::WA_DeleteOnClose);
				sr->setWindowTitle(s0);
				const QString s1 =
					SRUtils::read_sr_content_sq(
						ds,
						t00080005,
						fi.absolutePath(),
						static_cast<QWidget*>(settingswidget),
						sr->textBrowser,
						pb,
						sr->tmpfiles,
						sr->srimages,
						0,
						srinfo,
						QString("1"),
						true);
				sr->initSR(s1);
				sr->show();
#if (defined(__APPLE__) || defined(_WIN32))
				sr->activateWindow();
#endif
				sr->raise();
			}
			catch (const mdcm::ParseException & pe)
			{
#ifdef ALIZA_VERBOSE
				std::cout << pe.GetLastElement().GetTag() << std::endl;
#else
				(void)pe;
#endif
			}
			catch (const std::exception & ex)
			{
#ifdef ALIZA_VERBOSE
				std::cout << ex.what() << std::endl;
#else
				(void)ex;
#endif
			}
		}
	}
	//
	if (!pdf_files.empty())
	{
		for (int k = 0; k < pdf_files.size(); ++k)
		{
			if (pb) pb->hide();
			const QString pdff = QFileDialog::getSaveFileName(
				nullptr,
				QString("Select file"),
				CommonUtils::get_save_dir() +
					QString("/") +
					CommonUtils::get_save_name() +
					QString(".pdf"),
				QString("All Files (*)"),
				nullptr
				 //, QFileDialog::DontUseNativeDialog
				);
			if (!pdff.isEmpty())
			{
				if (pb) pb->show();
				QFileInfo pfi(pdff);
				CommonUtils::set_save_dir(pfi.absolutePath());
				try
				{
					DicomUtils::write_encapsulated(pdf_files.at(k), pdff);
				}
				catch (const mdcm::ParseException & pe)
				{
#ifdef ALIZA_VERBOSE
					std::cout << pe.GetLastElement().GetTag() << std::endl;
#else
					(void)pe;
#endif
				}
				catch (const std::exception & ex)
				{
#ifdef ALIZA_VERBOSE
					std::cout << ex.what() << std::endl;
#else
					(void)ex;
#endif
				}
			}
		}
	}
	//
	if (!stl_files.empty())
	{
		for (int k = 0; k < stl_files.size(); ++k)
		{
			if (pb) pb->hide();
			const QString stlf = QFileDialog::getSaveFileName(
				nullptr,
				QString("Select file"),
				CommonUtils::get_save_dir() +
					QString("/") +
					CommonUtils::get_save_name() +
					QString(".stl"),
				QString("All Files (*)"),
				nullptr
				 //, QFileDialog::DontUseNativeDialog
				);
			if (!stlf.isEmpty())
			{
				if (pb) pb->show();
				QFileInfo sfi(stlf);
				CommonUtils::set_save_dir(sfi.absolutePath());			
				try
				{
					DicomUtils::write_encapsulated(stl_files.at(k), stlf);
				}
				catch (const mdcm::ParseException & pe)
				{
#ifdef ALIZA_VERBOSE
					std::cout << pe.GetLastElement().GetTag() << std::endl;
#else
					(void)pe;
#endif
				}
				catch (const std::exception & ex)
				{
#ifdef ALIZA_VERBOSE
					std::cout << ex.what() << std::endl;
#else
					(void)ex;
#endif
				}
			}
		}
	}
	//
	if (!video_files.empty())
	{
		for (int k = 0; k < video_files.size(); ++k)
		{
			if (pb) pb->hide();
			const QString tmp943 = video_files.at(k);
			const QString suf = DicomUtils::suffix_mpeg(tmp943);
			const QString video_file_name =
				QFileDialog::getSaveFileName(
					nullptr,
					QString("Select file"),
					CommonUtils::get_save_dir() +
						QString("/") +
						CommonUtils::get_save_name() +
						suf,
					QString("All Files (*)"),
					nullptr
					 //, QFileDialog::DontUseNativeDialog
					);
			if (!video_file_name.isEmpty())
			{
				if (pb) pb->show();
				QFileInfo vfi(video_file_name);
				CommonUtils::set_save_dir(vfi.absolutePath());
				try
				{
					DicomUtils::write_mpeg(tmp943, video_file_name);
				}
				catch (const mdcm::ParseException & pe)
				{
#ifdef ALIZA_VERBOSE
					std::cout << pe.GetLastElement().GetTag() << std::endl;
#else
					(void)pe;
#endif
				}
				catch (const std::exception & ex)
				{
#ifdef ALIZA_VERBOSE
					std::cout << ex.what() << std::endl;
#else
					(void)ex;
#endif
				}
			}
		}
	}
#ifdef ALIZA_PRINT_COUNT_GL_OBJ
	std::cout << "Num VBOs " << GLWidget::get_count_vbos() << std::endl;
#endif
	return message;
}

void Aliza::set_adjust_glwidget(bool t)
{
	if (t)
	{
		glwidget->set_adjust(settingswidget->adjust_spinBox->value());
	}
	else
	{
		glwidget->set_adjust(0);
	}
}

void Aliza::set_adjust_value_glwidget(int x)
{
	if (settingswidget->adjust_checkBox->isChecked())
	{
		glwidget->set_adjust(x);
	}
}

#ifdef ALIZA_PRINT_COUNT_GL_OBJ
#undef ALIZA_PRINT_COUNT_GL_OBJ
#endif
