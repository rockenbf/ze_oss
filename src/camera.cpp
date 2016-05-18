#include <ze/cameras/camera.h>

#include <string>
#include <ze/cameras/camera_yaml_serialization.h>

namespace ze {

Camera::Camera(const uint32_t width, const uint32_t height, const CameraType type,
               const VectorX& projection_params, const VectorX& distortion_params)
  : size_(width, height)
  , projection_params_(projection_params)
  , distortion_params_(distortion_params)
  , type_(type)
{
  CHECK_EQ(projection_params_.size(), 4);
  switch (type_)
  {
    case CameraType::Pinhole:
      CHECK_EQ(distortion_params_.size(), 1);
      break;
    case CameraType::PinholeRadialTangential:
      CHECK_EQ(distortion_params_.size(), 4);
      break;
    case CameraType::PinholeEquidistant:
      CHECK_EQ(distortion_params_.size(), 4);
      break;
    case CameraType::PinholeFov:
    {
      CHECK_EQ(distortion_params_.size(), 1);
      // Pre-computations for improved speed.
      const FloatType s = distortion_params_(0);
      const FloatType tan_s_half_x2 = std::tan(s / 2.0) * 2.0;
      distortion_params_ = Vector2(s, tan_s_half_x2);
      break;
    }
    default:
      LOG(FATAL) << "Unknown camera model.";
      break;
  }
}

Bearings Camera::backProjectVectorized(const Eigen::Ref<const Keypoints>& px_vec) const
{
  Bearings bearings(3, px_vec.cols());
  for(int i = 0; i < px_vec.cols(); ++i)
  {
    bearings.col(i) = this->backProject(px_vec.col(i));
  }
  return bearings;
}

Keypoints Camera::projectVectorized(const Eigen::Ref<const Bearings>& bearing_vec) const
{
  Keypoints px_vec(2, bearing_vec.cols());
  for(int i = 0; i < bearing_vec.cols(); ++i)
  {
    px_vec.col(i) = this->project(bearing_vec.col(i));
  }
  return px_vec;
}

Matrix6X Camera::dProject_dLandmarkVectorized(const Positions& pos_vec) const
{
  Matrix6X J_vec(6, pos_vec.cols());
  for(int i = 0; i < pos_vec.cols(); ++i)
  {
    J_vec.col(i) =
        Eigen::Map<Matrix61>(this->dProject_dLandmark(pos_vec.col(i)).data());
  }
  return J_vec;
}

Camera::Ptr Camera::loadFromYaml(const std::string& path)
{
  try
  {
    YAML::Node doc = YAML::LoadFile(path.c_str());
    return doc.as<Camera::Ptr>();
  }
  catch (const std::exception& ex)
  {
    LOG(ERROR) << "Failed to load Camera from file " << path << " with the error: \n"
               << ex.what();
  }
  return Camera::Ptr();
}

std::string Camera::typeAsString() const
{
  switch (type_)
  {
    case CameraType::Pinhole: return "Pinhole";
    case CameraType::PinholeFov: return "PinholeFov";
    case CameraType::PinholeEquidistant: return "PinholeEquidistant";
    case CameraType::PinholeRadialTangential: return "PinholeRadialTangential";
    default:
      LOG(FATAL) << "Unknown parameter type";
  }
  return "";
}

void Camera::setMask(const Image8uC1::Ptr& mask)
{
  CHECK_NOTNULL(mask.get());
  CHECK_EQ(mask->size(), size_);
  mask_ = mask;
}

std::ostream& operator<<(std::ostream& out, const Camera& cam)
{
  out << "    Label = " << cam.label() << "\n"
      << "    Model = " << cam.typeAsString() << "\n"
      << "    Dimensions = " << cam.width() << "x" << cam.height() << "\n"
      << "    Proj. parameters = " << cam.projectionParameters().transpose() << "\n"
      << "    Dist. parameters = " << cam.distortionParameters().transpose() << "\n"
      << "    Masked = " << (cam.mask() ? "True" : "False");
  return out;
}

} // namespace ze
